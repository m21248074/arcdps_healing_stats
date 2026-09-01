#include "UpdateGUI.h"

#include "Exports.h"
#include "ImGuiEx.h"
#include "Log.h"

#include <shellapi.h>
#include <cpr/cpr.h>

void UpdateChecker::Log(std::string&& pMessage)
{
	LogI("{}", pMessage);
}

bool UpdateChecker::HttpDownload(const std::string& pUrl, const std::filesystem::path& pOutputFile)
{
	std::ofstream outputStream(pOutputFile, std::ios_base::out | std::ios_base::binary);
	cpr::Response response = cpr::Download(outputStream, cpr::Url{pUrl});
	if (response.status_code != 200) {
		Log(std::format("Downloading {} failed - http failure {} {}", pUrl, response.status_code,
		                response.status_line));
		return false;
	}

	return true;
}

std::optional<std::string> UpdateChecker::HttpGet(const std::string& pUrl)
{
	cpr::Response response = cpr::Get(cpr::Url{pUrl});
	if (response.status_code != 200) {
		Log(std::format("Getting {} failed - {} {}", pUrl, response.status_code, response.status_line));
		return std::nullopt;
	}

	return response.text;
}

void Display_UpdateWindow()
{
	auto& state = GlobalObjects::UPDATE_STATE;
	if (state == nullptr)
	{
		return;
	}

	std::lock_guard lock(state->Lock);
	if (state->UpdateStatus != UpdateChecker::Status::Unknown && state->UpdateStatus != UpdateChecker::Status::Dismissed)
	{
		bool shown = true;
		if (ImGui::Begin(
			"治療統計 更新###HEALING_STATS_UPDATE",
			&shown,
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize) == true)
		{
			const UpdateChecker::Version& currentVersion = *state->CurrentVersion;
			const UpdateChecker::Version& newVersion = state->NewVersion;

			ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "治療統計 插件有新的更新可用");
			ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "當前版本: %u.%urc%u", currentVersion[0], currentVersion[1], currentVersion[2]);
			ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "新版本: %u.%urc%u", newVersion[0], newVersion[1], newVersion[2]);

			if (ImGui::Button("開啟下載頁面") == true)
			{
				ShellExecuteA(nullptr, nullptr, "https://github.com/Krappa322/arcdps_healing_stats/releases", nullptr, nullptr, SW_SHOW);
			}

			switch (state->UpdateStatus)
			{
			case UpdateChecker::Status::UpdateAvailable:
				if (ImGui::Button("自動更新") == true)
				{
					GlobalObjects::UPDATE_CHECKER->PerformInstallOrUpdate(*state);
				}
				break;
			case UpdateChecker::Status::UpdateInProgress:
				ImGui::TextUnformatted("更新進行中");
				break;
			case UpdateChecker::Status::UpdateSuccessful:
				ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "更新完成，請重啟 激戰2 以使更新生效");
				break;
			case UpdateChecker::Status::UpdateError:
				ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "更新時發生錯誤");
				break;
			default:
				break;
			}

			ImGui::End();
		}

		if (shown != true)
		{
			state->UpdateStatus = ArcdpsExtension::UpdateCheckerBase::Status::Dismissed;
		}
	}
}
