#pragma once
#include "AggregatedStatsCollection.h"
#include "State.h"

#include <Windows.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class OffloadedStatsAggregation
{
	static constexpr size_t MAX_AGGREGATION_SOURCES = 10;
	static constexpr uint64_t AGGREGATION_INTERVAL_MS = 1000;
	using GetAggregationSourcesCb = std::function<std::vector<HealWindowOptions>(bool& pDebugMode)>;

public:
	explicit OffloadedStatsAggregation(GetAggregationSourcesCb&& pCallback);
	~OffloadedStatsAggregation();

	OffloadedStatsAggregation(const OffloadedStatsAggregation&) = delete;
	OffloadedStatsAggregation& operator=(const OffloadedStatsAggregation&) = delete;
	OffloadedStatsAggregation(OffloadedStatsAggregation&&) = delete;
	OffloadedStatsAggregation& operator=(OffloadedStatsAggregation&&) = delete;

	static void ThreadStartServe(void* pThis);
	void WakeThread() const;
	void Shutdown();
	std::unique_ptr<AggregatedStatsCollection> TryGetAggregatedStats(size_t pWindowIndex);

private:
	void Serve();

	std::atomic<bool> mIsShutdown = false;
	HANDLE mTimer;
	const GetAggregationSourcesCb mCallback;
	std::array<std::atomic<AggregatedStatsCollection*>, MAX_AGGREGATION_SOURCES> mAggregatedWindows{};
};
