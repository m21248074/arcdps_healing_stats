#include "OffloadedStatsAggregation.h"

#include "Exports.h"
#include "Log.h"
#include "State.h"

#include <memory>
#include <utility>
#include <vector>

OffloadedStatsAggregation::OffloadedStatsAggregation(GetAggregationSourcesCb&& pCallback)
	: mCallback{std::move(pCallback)}
{
	mTimer = CreateWaitableTimer(nullptr, FALSE, nullptr);
	if (mTimer == NULL)
	{
		LogC("Failed to create timer - {}", GetLastError());
	}
}

OffloadedStatsAggregation::~OffloadedStatsAggregation()
{
	for (std::atomic<AggregatedStatsCollection*>& c : mAggregatedWindows)
	{
		AggregatedStatsCollection* inner = c.load(std::memory_order_relaxed);
		if (inner != nullptr)
		{
			delete inner;
		}
	}
	CloseHandle(mTimer);
}

void OffloadedStatsAggregation::ThreadStartServe(void* pThis)
{
	reinterpret_cast<OffloadedStatsAggregation*>(pThis)->Serve();
}

void OffloadedStatsAggregation::Serve()
{
	while (mIsShutdown.load(std::memory_order_relaxed) == false)
	{
		LogT("Aggregating stats");

		bool debugMode;
		std::vector<HealWindowOptions> sources = mCallback(debugMode);
		assert(sources.size() == mAggregatedWindows.size());

		for (size_t i = 0; i < sources.size(); i++)
		{
			AggregatedStatsCollection* collection = nullptr;
			if (sources[i].Shown == true)
			{
				auto [localId, states] = GlobalObjects::EVENT_PROCESSOR->GetState();
				collection = new AggregatedStatsCollection{std::move(states), localId, sources[i], debugMode};
			}

			// This is done even if we didn't make a new collection (i.e. collection is nullptr), so that we invalidate
			// aggregations of windows that are not visible
			AggregatedStatsCollection* old = mAggregatedWindows[i].exchange(collection, std::memory_order_relaxed);
			if (old != nullptr)
			{
				delete old;
			}
		}

		LARGE_INTEGER dueTime;
		// 100ns periods, negative means relative time
		dueTime.QuadPart = -static_cast<int64_t>(AGGREGATION_INTERVAL_MS * 1000 * 10);
		if (SetWaitableTimer(mTimer, &dueTime, 0, NULL, NULL, FALSE) != TRUE)
		{
			LogE("Failed to set period for offloaded stats timer - {}", GetLastError());
		}
		if (WaitForSingleObject(mTimer, INFINITE) != WAIT_OBJECT_0)
		{
			LogE("Failed to wait on offloaded stats timer - {}", GetLastError());
		}
	}
}

void OffloadedStatsAggregation::WakeThread() const
{
	LARGE_INTEGER dueTime;
	dueTime.QuadPart = 0;
	if (SetWaitableTimer(mTimer, &dueTime, 0, NULL, NULL, FALSE) != TRUE)
	{
		LogE("Failed to wake offloaded stats thread - {}", GetLastError());
	}
}

void OffloadedStatsAggregation::Shutdown()
{
	mIsShutdown.store(true, std::memory_order_relaxed);
	WakeThread();
}

std::unique_ptr<AggregatedStatsCollection> OffloadedStatsAggregation::TryGetAggregatedStats(size_t pWindowIndex)
{
	assert(pWindowIndex < mAggregatedWindows.size());

	// Fast path, don't congest the cacheline
	if (mAggregatedWindows[pWindowIndex].load(std::memory_order_relaxed) == nullptr)
	{
		return nullptr;
	}

	AggregatedStatsCollection* val = mAggregatedWindows[pWindowIndex].exchange(nullptr, std::memory_order_relaxed);
	return std::unique_ptr<AggregatedStatsCollection>{val};
}
