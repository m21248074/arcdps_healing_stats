#include "GUI.h"

#include "AggregatedStatsCollection.h"
#include "Exports.h"
#include "ImGuiEx.h"
#include "Log.h"
#include "Utilities.h"
#include "Widgets.h"

#include <array>
#include <Windows.h>

static constexpr EnumStringArray<AutoUpdateSettingEnum> AUTO_UPDATE_SETTING_ITEMS{
	u8"關閉", u8"開啟 (僅下載穩定版本)", u8"開啟 (下載先行/穩定版本)"
};
static constexpr EnumStringArray<DataSource> DATA_SOURCE_ITEMS{
	u8"目標", u8"技能", u8"總計", u8"綜合", u8"團員輸出治療"};
static constexpr EnumStringArray<SortOrder> SORT_ORDER_ITEMS{
	u8"按字母由A到Z(升序)", u8"按字母由Z到A(降序)", u8"按每秒治療量由小到大(升序)", u8"按每秒治療量由大到小(降序)"};
static constexpr EnumStringArray<CombatEndCondition> COMBAT_END_CONDITION_ITEMS{
	u8"離開戰鬥", u8"最後傷害事件", u8"最後治療事件", u8"最後傷害/治療事件"};
static constexpr EnumStringArray<spdlog::level::level_enum, 7> LOG_LEVEL_ITEMS{
	u8"追蹤", u8"除錯", u8"資訊", u8"警告", u8"錯誤", u8"致命", u8"關閉"};

static constexpr EnumStringArray<Position, static_cast<size_t>(Position::WindowRelative) + 1> POSITION_ITEMS{
	u8"手動", u8"螢幕相對位置", u8"視窗相對位置"};
static constexpr EnumStringArray<CornerPosition, static_cast<size_t>(CornerPosition::BottomRight) + 1> CORNER_POSITION_ITEMS{
	u8"左上", u8"右上", u8"左下", u8"右下"};

static void Display_DetailsWindow(HealWindowContext& pContext, DetailsWindowState& pState, DataSource pDataSource)
{
	if (pState.IsOpen == false)
	{
		return;
	}

	char buffer[1024];
	// Using "###" means the id of the window is calculated only from the part after the hashes (which
	// in turn means that the name of the window can change if necessary)
	snprintf(buffer, sizeof(buffer), "%s###HEALDETAILS.%i.%llu", pState.Name.c_str(), static_cast<int>(pDataSource), pState.Id);
	ImGui::SetNextWindowSize(ImVec2(600, 360), ImGuiCond_FirstUseEver);
	ImGui::Begin(buffer, &pState.IsOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus);

	if (ImGui::BeginPopupContextWindow("Options##HEAL") == true)
	{
		ImGui::InputText(u8"條目格式", pContext.DetailsEntryFormat, sizeof(pContext.DetailsEntryFormat));
		ImGuiEx::AddTooltipToLastItem(u8"顯示資料的格式(統計資料為每個條目)。\n"
		                              u8"{1}: 治療量\n"
		                              u8"{2}: 命中數\n"
		                              u8"{3}: 施法數 (尚未實作)\n"
		                              u8"{4}: 每秒治療量\n"
		                              u8"{5}: 每次命中治療量\n"
		                              u8"{6}: 每次施法治療量 (尚未實作)\n"
		                              u8"{7}: 佔總治療量百分比\n");

		ImGui::EndPopup();
	}

	ImVec4 bgColor = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
	bgColor.w = 0.0f;
	ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
	snprintf(buffer, sizeof(buffer), "##HEALDETAILS.TOTALS.%i.%llu", static_cast<int>(pDataSource), pState.Id);
	ImGui::BeginChild(buffer, ImVec2(ImGui::GetWindowContentRegionWidth() * 0.35f, 0));
	ImGui::Text(u8"總治療量");
	ImGuiEx::TextRightAlignedSameLine("%llu", pState.Healing);

	ImGui::Text(u8"命中數");
	ImGuiEx::TextRightAlignedSameLine("%llu", pState.Hits);

	if (pState.Casts.has_value() == true)
	{
		ImGui::Text(u8"施法數");
		ImGuiEx::TextRightAlignedSameLine("%llu", *pState.Casts);
	}

	ImGui::Text(u8"每秒治療量");
	ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(pState.Healing, pState.TimeInCombat));

	ImGui::Text(u8"每次命中治療量");
	ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(pState.Healing, pState.Hits));

	if (pState.Casts.has_value() == true)
	{
		ImGui::Text(u8"每次施法治療量");
		ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(pState.Healing, *pState.Casts));
	}

	ImGuiEx::BottomText("ID %u", pState.Id);
	ImGui::EndChild();

	snprintf(buffer, sizeof(buffer), "##HEALDETAILS.ENTRIES.%i.%llu", static_cast<int>(pDataSource), pState.Id);
	ImGui::SameLine();
	ImGui::BeginChild(buffer, ImVec2(0, 0));

	const AggregatedVector& stats = pContext.CurrentAggregatedStats->GetDetails(pDataSource, pState.Id);
	for (const auto& entry : stats.Entries)
	{
		std::array<std::optional<std::variant<uint64_t, double>>, 7> entryValues{
			entry.Healing,
			entry.Hits,
			entry.Casts,
			divide_safe(entry.Healing, entry.TimeInCombat),
			divide_safe(entry.Healing, entry.Hits),
			entry.Casts.has_value() == true ? std::optional{divide_safe(entry.Healing, *entry.Casts)} : std::nullopt,
			divide_safe(entry.Healing * 100, pState.Healing)};
		ReplaceFormatted(buffer, sizeof(buffer), pContext.DetailsEntryFormat, entryValues);

		float fillRatio = static_cast<float>(divide_safe(entry.Healing, stats.HighestHealing));
		std::string_view name = entry.Name;
		if (pContext.MaxNameLength > 0)
		{
			name = name.substr(0, pContext.MaxNameLength);
		}
		ImGuiEx::StatsEntry(name, buffer, pContext.ShowProgressBars == true ? std::optional{fillRatio} : std::nullopt);
	}
	ImGui::EndChild();

	ImGui::PopStyleColor();
	ImGui::End();
}

static void Display_Content(HealWindowContext& pContext, DataSource pDataSource, uint32_t pWindowIndex, bool pEvtcRpcEnabled)
{
	UNREFERENCED_PARAMETER(pWindowIndex);
	char buffer[1024];

	if (pDataSource == DataSource::PeersOutgoing)
	{
		if (pEvtcRpcEnabled == false)
		{
			ImGui::TextWrapped(u8"即時統計資料共享已停用。 啟用\"治療統計選項\"內的\"即時統計資料共享\"，以便查看團隊中其他玩家的治療狀況。");
			pContext.CurrentFrameLineCount += 3;
			return;
		}

		evtc_rpc_client_status status = GlobalObjects::EVTC_RPC_CLIENT->GetStatus();
		if (status.Connected == false)
		{
			ImGui::TextWrapped(u8"未連接到即時統計共享伺服器 (\"%s\")。", status.Endpoint.c_str());
			pContext.CurrentFrameLineCount += 3;
			return;
		}
	}

	const AggregatedStatsEntry& aggregatedTotal = pContext.CurrentAggregatedStats->GetTotal(pDataSource);

	const AggregatedVector& stats = pContext.CurrentAggregatedStats->GetStats(pDataSource);
	for (size_t i = 0; i < stats.Entries.size(); i++)
	{
		const auto& entry = stats.Entries[i];

		std::array<std::optional<std::variant<uint64_t, double>>, 7> entryValues{
			entry.Healing,
			entry.Hits,
			entry.Casts,
			divide_safe(entry.Healing, entry.TimeInCombat),
			divide_safe(entry.Healing, entry.Hits),
			entry.Casts.has_value() == true ? std::optional{divide_safe(entry.Healing, *entry.Casts)} : std::nullopt,
			pContext.DataSourceChoice != DataSource::Totals ? std::optional{divide_safe(entry.Healing * 100, aggregatedTotal.Healing)} : std::nullopt };
		ReplaceFormatted(buffer, sizeof(buffer), pContext.EntryFormat, entryValues);

		float fillRatio = static_cast<float>(divide_safe(entry.Healing, stats.HighestHealing));
		std::string_view name = entry.Name;
		if (pContext.MaxNameLength > 0)
		{
			name = name.substr(0, pContext.MaxNameLength);
		}
		float minSize = ImGuiEx::StatsEntry(name, buffer, pContext.ShowProgressBars == true ? std::optional{fillRatio} : std::nullopt);
		pContext.LastFrameMinWidth = (std::max)(pContext.LastFrameMinWidth, minSize);
		pContext.CurrentFrameLineCount += 1;

		DetailsWindowState* state = nullptr;
		std::vector<DetailsWindowState>* vec;
		switch (pDataSource)
		{
		case DataSource::Agents:
			vec = &pContext.OpenAgentWindows;
			break;
		case DataSource::Skills:
			vec = &pContext.OpenSkillWindows;
			break;
		case DataSource::PeersOutgoing:
			vec = &pContext.OpenPeersOutgoingWindows;
			break;
		default:
			vec = nullptr;
			break;
		}

		if (vec != nullptr)
		{
			for (auto& iter : *vec)
			{
				if (iter.Id == entry.Id)
				{
					state = &(iter);
					break;
				}
			}
		}

		// If it was opened in a previous frame, we need the update the statistics stored so they are up to date
		if (state != nullptr)
		{
			*static_cast<AggregatedStatsEntry*>(state) = entry;
		}

		if (vec != nullptr && ImGui::IsItemClicked() == true)
		{
			if (state == nullptr)
			{
				state = &vec->emplace_back(entry);
			}
			state->IsOpen = !state->IsOpen;

			LOG("Toggled details window for entry %llu %s in window %u", entry.Id, entry.Name.c_str(), pWindowIndex);
		}
	}
}

static void Display_WindowOptions_Position(HealTableOptions& pHealingOptions, HealWindowContext& pContext)
{
	ImGuiEx::SmallEnumRadioButton("PositionEnum", pContext.PositionRule, POSITION_ITEMS);

	switch (pContext.PositionRule)
	{
		case Position::ScreenRelative:
		{
			ImGui::Separator();
			ImGui::TextUnformatted(u8"相對於螢幕角落");
			ImGuiEx::SmallIndent();
			ImGuiEx::SmallEnumRadioButton("ScreenCornerPositionEnum", pContext.RelativeScreenCorner, CORNER_POSITION_ITEMS);
			ImGuiEx::SmallUnindent();

			ImGui::PushItemWidth(39.0f);
			ImGuiEx::SmallInputInt("x", &pContext.RelativeX);
			ImGuiEx::SmallInputInt("y", &pContext.RelativeY);
			ImGui::PopItemWidth();

			break;
		}
		case Position::WindowRelative:
		{
			ImGui::Separator();
			ImGui::TextUnformatted(u8"從錨點面板角落");
			ImGuiEx::SmallIndent();
			ImGuiEx::SmallEnumRadioButton("AnchorCornerPositionEnum", pContext.RelativeAnchorWindowCorner, CORNER_POSITION_ITEMS);
			ImGuiEx::SmallUnindent();

			ImGui::TextUnformatted(u8"到此面板角落");
			ImGuiEx::SmallIndent();
			ImGuiEx::SmallEnumRadioButton("SelfCornerPositionEnum", pContext.RelativeSelfCorner, CORNER_POSITION_ITEMS);
			ImGuiEx::SmallUnindent();

			ImGui::PushItemWidth(39.0f);
			ImGuiEx::SmallInputInt("x", &pContext.RelativeX);
			ImGuiEx::SmallInputInt("y", &pContext.RelativeY);
			ImGui::PopItemWidth();

			ImGuiWindow* selectedWindow = ImGui::FindWindowByID(pContext.AnchorWindowId);
			const char* selectedWindowName = "";
			if (selectedWindow != nullptr)
			{
				selectedWindowName = selectedWindow->Name;
			}

			ImGui::SetNextItemWidth(260.0f);
			if (ImGui::BeginCombo(u8"錨點視窗", selectedWindowName) == true)
			{
				// This doesn't return the same thing as RootWindow interestingly enough, RootWindow returns a "higher" parent
				ImGuiWindow* parent = ImGui::GetCurrentWindowRead();
				while (parent->ParentWindow != nullptr)
				{
					parent = parent->ParentWindow;
				}

				for (ImGuiWindow* window : ImGui::GetCurrentContext()->Windows)
				{
					if (window != parent && // Not the window we're currently in
						window->ParentWindow == nullptr && // Not a child window of another window
						window->Hidden == false && // Not hidden
						window->WasActive == true && // Not closed (we check ->WasActive because ->Active might not be true yet if the window gets rendered after this one)
						window->IsFallbackWindow == false && // Not default window ("Debug##Default")
						(window->Flags & ImGuiWindowFlags_Tooltip) == 0) // Not a tooltip window ("##Tooltip_<id>")
					{
						std::string windowName = "(";
						windowName += std::to_string(pHealingOptions.AnchoringHighlightedWindows.size());
						windowName += ") ";
						windowName += window->Name;

						// Print the window with ##/### part still there - it doesn't really hurt (and with the presence
						// of ### it's arguably more correct, even though it probably doesn't matter for a Selectable if
						// the id is unique or not)
						if (ImGui::Selectable(windowName.c_str()))
						{
							pContext.AnchorWindowId = window->ID;
							FindAndResolveCyclicDependencies(pHealingOptions, std::distance(pHealingOptions.Windows.data(), &pContext));
						}

						pHealingOptions.AnchoringHighlightedWindows.emplace_back(window->ID);
					}
				}
				ImGui::EndCombo();
			}

			break;
		}
		default:
			break;
	}
}

static void Display_WindowOptions(HealTableOptions& pHealingOptions, HealWindowContext& pContext)
{
	if (ImGui::BeginPopupContextWindow("Options##HEAL") == true)
	{
		ImGuiEx::SmallIndent();
		ImGuiEx::ComboMenu(u8"資料來源", pContext.DataSourceChoice, DATA_SOURCE_ITEMS);
		ImGuiEx::AddTooltipToLastItem(u8"決定視窗中顯示哪些資料");

		if (pContext.DataSourceChoice != DataSource::Totals)
		{
			ImGuiEx::ComboMenu(u8"排序", pContext.SortOrderChoice, SORT_ORDER_ITEMS);
			ImGuiEx::AddTooltipToLastItem(u8"決定如何在\"目標\"和\"技能\"部分中對目標和技能進行排序。");

			if (ImGui::BeginMenu(u8"統計資料排除") == true)
			{
				ImGuiEx::SmallCheckBox(u8"小隊", &pContext.ExcludeGroup);
				ImGuiEx::SmallCheckBox(u8"小隊外", &pContext.ExcludeOffGroup);
				ImGuiEx::SmallCheckBox(u8"團隊外", &pContext.ExcludeOffSquad);
				ImGuiEx::SmallCheckBox(u8"召喚物", &pContext.ExcludeMinions);
				ImGuiEx::SmallCheckBox(u8"其他", &pContext.ExcludeUnmapped);

				ImGui::EndMenu();
			}
		}

		ImGuiEx::ComboMenu(u8"戰鬥結束", pContext.CombatEndConditionChoice, COMBAT_END_CONDITION_ITEMS);
		ImGuiEx::AddTooltipToLastItem(u8"決定應該使用標準來確定戰鬥結束\n"
										u8"(以及戰鬥中的時間)");

		ImGuiEx::SmallUnindent();
		ImGui::Separator();

		if (ImGui::BeginMenu(u8"顯示") == true)
		{
			ImGuiEx::SmallCheckBox(u8"顯示條形圖", &pContext.ShowProgressBars);
			ImGuiEx::AddTooltipToLastItem(u8"在每個條目下方顯示一個有色條，表示該條目與最大條目的比例");

			ImGui::SetNextItemWidth(260.0f);
			ImGuiEx::SmallInputText(u8"簡稱", pContext.Name, sizeof(pContext.Name));
			ImGuiEx::AddTooltipToLastItem(u8"用於在\"治療統計\"選單中表示此視窗的名稱");

			ImGui::SetNextItemWidth(39.0f);
			ImGuiEx::SmallInputInt(u8"最大名字長度", &pContext.MaxNameLength);
			ImGuiEx::AddTooltipToLastItem(
				u8"將顯示的名稱截斷為所設定長度的字串。 設定為 0 以停用。");

			ImGui::SetNextItemWidth(39.0f);
			ImGuiEx::SmallInputInt(u8"最小顯示行數", &pContext.MinLinesDisplayed);
			ImGuiEx::AddTooltipToLastItem(
				u8"此視窗中顯示的最小資料行數。");

			ImGui::SetNextItemWidth(39.0f);
			ImGuiEx::SmallInputInt(u8"最大顯示行數", &pContext.MaxLinesDisplayed);
			ImGuiEx::AddTooltipToLastItem(
				u8"此視窗中顯示的最大資料行數。 設定為 0 表示無限制");

			ImGui::SetNextItemWidth(260.0f);
			ImGuiEx::SmallInputText(u8"統計資料格式", pContext.EntryFormat, sizeof(pContext.EntryFormat));
			if (pContext.DataSourceChoice != DataSource::Totals)
			{
				ImGuiEx::AddTooltipToLastItem(u8"顯示資料的格式(統計資料為每個條目)。\n"
					u8"{1}: 治療量\n"
					u8"{2}: 命中數\n"
					u8"{3}: 施法數 (尚未實作)\n"
					u8"{4}: 每秒治療量\n"
					u8"{5}: 每次命中治療量\n"
					u8"{6}: 每次施法治療量 (尚未實作)\n"
					u8"{7}: 佔總治療量百分比");
			}
			else
			{
				ImGuiEx::AddTooltipToLastItem(u8"顯示資料的格式(統計資料為每個條目)。\n"
					u8"{1}: 治療量\n"
					u8"{2}: 命中數\n"
					u8"{3}: 施法數 (尚未實作)\n"
					u8"{4}: 每秒治療量\n"
					u8"{5}: 每次命中治療量\n"
					u8"{6}: 每次施法治療量 (尚未實作)");
			}

			ImGui::SetNextItemWidth(260.0f);
			ImGuiEx::SmallInputText(u8"標題欄格式", pContext.TitleFormat, sizeof(pContext.TitleFormat));
			if (pContext.DataSourceChoice != DataSource::Totals)
			{
				ImGuiEx::AddTooltipToLastItem(u8"此視窗標題的格式。\n"
					u8"{1}: 總治療量\n"
					u8"{2}: 總命中數\n"
					u8"{3}: 總施法數 (尚未實作)\n"
					u8"{4}: 每秒治療量\n"
					u8"{5}: 每次命中治療量\n"
					u8"{6}: 每次施法治療量 (尚未實作)\n"
					u8"{7}: 戰鬥時間");
			}
			else
			{
				ImGuiEx::AddTooltipToLastItem(u8"此視窗標題的格式。\n"
					u8"{1}: 戰鬥時間");
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(u8"樣式") == true)
		{
			ImGuiEx::SmallEnumCheckBox(u8"標題欄", &pContext.WindowFlags, ImGuiWindowFlags_NoTitleBar, true);
			ImGuiEx::SmallEnumCheckBox(u8"滾動條", &pContext.WindowFlags, ImGuiWindowFlags_NoScrollbar, true);
			ImGuiEx::SmallEnumCheckBox(u8"背景", &pContext.WindowFlags, ImGuiWindowFlags_NoBackground, true);
			
			ImGui::Separator();

			ImGuiEx::SmallCheckBox(u8"自動調整視窗大小", &pContext.AutoResize);
			if (pContext.AutoResize == false)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(128, 128, 128, 255));
			}
			ImGuiEx::SmallIndent();

			ImGui::SetNextItemWidth(65.0f);
			ImGuiEx::SmallInputInt(u8"視窗寬度", &pContext.FixedWindowWidth);
			ImGuiEx::AddTooltipToLastItem(
				u8"設定為 0 以動態調整寬度大小");

			ImGuiEx::SmallUnindent();
			if (pContext.AutoResize == false)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleColor();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(u8"位置座標") == true)
		{
			Display_WindowOptions_Position(pHealingOptions, pContext);
			ImGui::EndMenu();
		}

		ImGui::Separator();

		float oldPosY = ImGui::GetCursorPosY();
		ImGui::BeginGroup();

		ImGui::SetCursorPosY(oldPosY + ImGui::GetStyle().FramePadding.y);
		ImGui::Text(u8"熱鍵");

		ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
		ImGui::SetCursorPosY(oldPosY);

		ImGui::SetNextItemWidth(ImGui::CalcTextSize("-----").x);
		ImGui::InputInt("##HOTKEY", &pContext.Hotkey, 0);

		ImGui::SameLine();
		ImGui::Text("(%s)", VirtualKeyToString(pContext.Hotkey).c_str());

		ImGui::EndGroup();
		ImGuiEx::AddTooltipToLastItem(u8"用於開啟和關閉此視窗按鍵的數值\n"
										u8"(虛擬按鍵代碼)");

		ImGui::EndPopup();
	}
}

void SetContext(void* pImGuiContext)
{
	ImGui::SetCurrentContext((ImGuiContext*)pImGuiContext);
}

void Display_GUI(HealTableOptions& pHealingOptions)
{
	char buffer[1024];

	for (uint32_t i = 0; i < HEAL_WINDOW_COUNT; i++)
	{
		HealWindowContext& curWindow = pHealingOptions.Windows[i];

		if (curWindow.Shown == false)
		{
			continue;
		}

		time_t curTime = ::time(0);
		if (curWindow.LastAggregatedTime < curTime)
		{
			//LOG("Fetching new aggregated stats");

			auto [localId, states] = GlobalObjects::EVENT_PROCESSOR->GetState();
			curWindow.CurrentAggregatedStats = std::make_unique<AggregatedStatsCollection>(std::move(states), localId, curWindow, pHealingOptions.DebugMode);
			curWindow.LastAggregatedTime = curTime;
		}

		float timeInCombat = curWindow.CurrentAggregatedStats->GetCombatTime();
		const AggregatedStatsEntry& aggregatedTotal = curWindow.CurrentAggregatedStats->GetTotal(curWindow.DataSourceChoice);

		if (curWindow.DataSourceChoice != DataSource::Totals)
		{
			std::array<std::optional<std::variant<uint64_t, double>>, 7> titleValues{
				aggregatedTotal.Healing,
				aggregatedTotal.Hits,
				aggregatedTotal.Casts,
				divide_safe(aggregatedTotal.Healing, aggregatedTotal.TimeInCombat),
				divide_safe(aggregatedTotal.Healing, aggregatedTotal.Hits),
				aggregatedTotal.Casts.has_value() == true ? std::optional{divide_safe(aggregatedTotal.Healing, *aggregatedTotal.Casts)} : std::nullopt,
				timeInCombat };
			size_t written = ReplaceFormatted(buffer, 128ULL, curWindow.TitleFormat, titleValues);
			snprintf(buffer + written, sizeof(buffer) - written, "###HEALWINDOW%u", i);
		}
		else
		{
			std::array<std::optional<std::variant<uint64_t, double>>, 7> titleValues{
			timeInCombat };
			size_t written = ReplaceFormatted(buffer, 128ULL, curWindow.TitleFormat, titleValues);
			snprintf(buffer + written, sizeof(buffer) - written, "###HEALWINDOW%u", i);
		}

		ImGuiWindowFlags window_flags = curWindow.WindowFlags | ImGuiWindowFlags_NoCollapse;
		if (curWindow.PositionRule != Position::Manual &&
			(curWindow.PositionRule != Position::WindowRelative || curWindow.AnchorWindowId != 0))
		{
			window_flags |= ImGuiWindowFlags_NoMove;
		}

		ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
		ImGui::Begin(buffer, &curWindow.Shown, window_flags);

		// Adjust x size. This is based on last frame width and has to happen before we draw anything since some items are
		// aligned to the right edge of the window.
		if (curWindow.AutoResize == true)
		{
			ImVec2 size = ImGui::GetCurrentWindowRead()->SizeFull;
			if (curWindow.FixedWindowWidth == 0)
			{
				if (curWindow.LastFrameMinWidth != 0)
				{
					size.x = curWindow.LastFrameMinWidth + ImGui::GetCurrentWindowRead()->ScrollbarSizes.x;
				}
				// else size.x = current size x
			}
			else
			{
				size.x = static_cast<float>(curWindow.FixedWindowWidth);
			}

			LogT("FixedWindowWidth={} LastFrameMinWidth={} x={}",
				curWindow.FixedWindowWidth, curWindow.LastFrameMinWidth, size.x);
			ImGui::SetWindowSize(size);
		}

		curWindow.LastFrameMinWidth = 0;
		curWindow.CurrentFrameLineCount = 0;

		curWindow.WindowId = ImGui::GetCurrentWindow()->ID;

		ImGui::SetNextWindowSize(ImVec2(170, 0));
		Display_WindowOptions(pHealingOptions, curWindow);

		if (curWindow.DataSourceChoice != DataSource::Combined)
		{
			Display_Content(curWindow, curWindow.DataSourceChoice, i, pHealingOptions.EvtcRpcEnabled);
		}
		else
		{
			curWindow.CurrentFrameLineCount += 3; // For the 3 headers

			ImGui::PushID(static_cast<int>(DataSource::Totals));
			ImGuiEx::TextColoredCentered(ImColor(0, 209, 165), u8"總計");
			Display_Content(curWindow, DataSource::Totals, i, pHealingOptions.EvtcRpcEnabled);
			ImGui::PopID();

			ImGui::PushID(static_cast<int>(DataSource::Agents));
			ImGuiEx::TextColoredCentered(ImColor(0, 209, 165), u8"目標");
			Display_Content(curWindow, DataSource::Agents, i, pHealingOptions.EvtcRpcEnabled);
			ImGui::PopID();

			ImGui::PushID(static_cast<int>(DataSource::Skills));
			ImGuiEx::TextColoredCentered(ImColor(0, 209, 165), u8"技能");
			Display_Content(curWindow, DataSource::Skills, i, pHealingOptions.EvtcRpcEnabled);
			ImGui::PopID();
		}

		for (const DataSource dataSource : std::array{DataSource::Agents, DataSource::Skills, DataSource::PeersOutgoing})
		{
			std::vector<DetailsWindowState>* vec;
			switch (dataSource)
			{
			case DataSource::Agents:
				vec = &curWindow.OpenAgentWindows;
				break;
			case DataSource::Skills:
				vec = &curWindow.OpenSkillWindows;
				break;
			case DataSource::PeersOutgoing:
				vec = &curWindow.OpenPeersOutgoingWindows;
				break;
			default:
				vec = nullptr;
				break;
			}

			auto iter = vec->begin();
			while (iter != vec->end())
			{
				if (iter->IsOpen == false)
				{
					iter = vec->erase(iter);
					continue;
				}

				Display_DetailsWindow(curWindow, *iter, dataSource);
				iter++;
			}
		}
		
		// Adjust y size. This is based on the current frame line count, so we do it at the end of the frame (meaning
		// the resize has no lag)
		if (curWindow.AutoResize == true)
		{
			ImVec2 size = ImGui::GetCurrentWindowRead()->SizeFull;

			size_t lineCount = (std::max)(curWindow.CurrentFrameLineCount, curWindow.MinLinesDisplayed);
			lineCount = (std::min)(lineCount, curWindow.MaxLinesDisplayed);
			
			size.y = ImGuiEx::CalcWindowHeight(lineCount);

			LogT("lineCount={} CurrentFrameLineCount={} MinLinesDisplayed={} MaxLinesDisplayed={} y={}",
				lineCount, curWindow.CurrentFrameLineCount, curWindow.MinLinesDisplayed, curWindow.MaxLinesDisplayed, size.y);

			ImGui::SetWindowSize(size);
		}
		ImGui::End();
	}
}

static void Display_EvtcRpcStatus(const HealTableOptions& pHealingOptions)
{
	evtc_rpc_client_status status = GlobalObjects::EVTC_RPC_CLIENT->GetStatus();
	if (status.Connected == true)
	{
		uint64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - status.ConnectTime).count();
		ImGui::TextColored(ImVec4(0.0f, 0.75f, 0.0f, 1.0f), u8"已連線到 %s %llu 秒", status.Endpoint.c_str(), seconds);
	}
	else if (pHealingOptions.EvtcRpcEnabled == false)
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), u8"由於未啟用即時統計資料共享而未進行連接");
	}
	else
	{
		ImGui::TextColored(ImVec4(0.75f, 0.0f, 0.0f, 1.0f), u8"無法連線到 %s。 重試中...", status.Endpoint.c_str());
	}
}

void Display_AddonOptions(HealTableOptions& pHealingOptions)
{
	ImGui::TextUnformatted(u8"治療統計");
	ImGuiEx::SmallIndent();
	for (uint32_t i = 0; i < HEAL_WINDOW_COUNT; i++)
	{
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "(%u) %s", i, pHealingOptions.Windows[i].Name);
		ImGuiEx::SmallCheckBox(buffer, &pHealingOptions.Windows[i].Shown);
	}
	ImGuiEx::SmallUnindent();

	ImGuiEx::ComboMenu(u8"自動更新", pHealingOptions.AutoUpdateSetting, AUTO_UPDATE_SETTING_ITEMS);
	ImGuiEx::SmallCheckBox(u8"除錯模式", &pHealingOptions.DebugMode);
	ImGuiEx::AddTooltipToLastItem(
		u8"在目標和技能名稱中包括除錯資料。\n"
		u8"在擷取潛在計算問題截圖前，打開此選項。");

	spdlog::string_view_t log_level_names[] = SPDLOG_LEVEL_NAMES;
	std::string log_level_items[std::size(log_level_names)];
	for (size_t i = 0; i < std::size(log_level_names); i++)
	{
		log_level_items[i] = std::string_view(log_level_names[i].data(), log_level_names[i].size());
	}

	if (ImGuiEx::ComboMenu(u8"除錯紀錄", pHealingOptions.LogLevel, LOG_LEVEL_ITEMS) == true)
	{
		Log_::SetLevel(pHealingOptions.LogLevel);
	}
	ImGuiEx::AddTooltipToLastItem(
		u8"如果未設定為關閉，將開啟特定紀錄嚴重性層級的日誌紀錄。\n"
		u8"日誌保存在 addons\\logs\\arcdps_healing_stats\\。\n"
		u8"日誌記錄將對效能造成一點影響。");

	ImGui::Separator();

	if (ImGuiEx::SmallCheckBox(u8"將治療紀錄到 EVTC 日誌裡", &pHealingOptions.EvtcLoggingEnabled) == true)
	{
		GlobalObjects::EVENT_PROCESSOR->SetEvtcLoggingEnabled(pHealingOptions.EvtcLoggingEnabled);
	}

	if (ImGuiEx::SmallCheckBox(u8"啟用即時統計資料共享", &pHealingOptions.EvtcRpcEnabled) == true)
	{
		GlobalObjects::EVTC_RPC_CLIENT->SetEnabledStatus(pHealingOptions.EvtcRpcEnabled);
	}
	ImGuiEx::AddTooltipToLastItem(
		u8"允許與小隊中的其他玩家即時共享治療統計資料。\n"
		u8"這是透過中央伺服器完成的(請參閱下面的選項)\n"
		u8"啟用即時共享後，在換地圖以便正確檢測到所有\n"
		u8"小隊成員之前，它可能無法正常工作\n"
		u8"\n"
		u8"可透過治療統計視窗查看其他玩家分享的統計資料\n"
		u8"，其資料來源設定為 \"%s\"\n"
		u8"\n"
		u8"啟用此選項將對效能造成一點影響，並且會對您的\n"
		u8"網路連線造成一些額外負載(最多約 10 kiB/s 上傳\n"
		u8"和 100 kiB/s 下載)。\n"
		u8"團隊中啟用即時統計資料共享的玩家越多，下載連線\n"
		u8"使用量就會增加，而上傳連線使用量則不會。"
		, DATA_SOURCE_ITEMS[DataSource::PeersOutgoing]);

	if (pHealingOptions.EvtcRpcEnabled == false)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(128, 128, 128, 255));
	}
	if (ImGuiEx::SmallCheckBox(u8"即時統計資料共享 節省模式", &pHealingOptions.EvtcRpcBudgetMode) == true)
	{
		GlobalObjects::EVTC_RPC_CLIENT->SetBudgetMode(pHealingOptions.EvtcRpcBudgetMode);
	}
	if (pHealingOptions.EvtcRpcEnabled == false)
	{
		ImGui::PopItemFlag();
		ImGui::PopStyleColor();
	}
	ImGuiEx::AddTooltipToLastItem(
		u8"只向團員發送事件的最小子集。 這減少了此插件\n"
		u8"使用的上傳頻寬量。 顯示的治療統計資料仍然完全\n"
		u8"準確，但是其他玩家在戰鬥中看到的戰鬥時間可能\n"
		u8"會稍微不準確。 如果這些玩家使用的是在引入節省\n"
		u8"模式支援之前發布的插件版本，則即使在非戰鬥狀態下\n"
		u8"，戰鬥時間也可能非常不準確。 此選項對下載頻寬\n"
		u8"使用量沒有影響，只影響上傳。 啟用此選項後，\n"
		u8"預期連線使用量應降於 < 1 kiB/s 上傳。");

	float oldPosY = ImGui::GetCursorPosY();
	ImGui::BeginGroup();

	ImGui::SetCursorPosY(oldPosY + ImGui::GetStyle().FramePadding.y);
	ImGui::Text(u8"即時統計資料共享 熱鍵");

	ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::SetCursorPosY(oldPosY);

	ImGui::SetNextItemWidth(ImGui::CalcTextSize("-----").x);
	ImGui::InputInt("##HOTKEY", &pHealingOptions.EvtcRpcEnabledHotkey, 0);

	ImGui::SameLine();
	ImGui::Text("(%s)", VirtualKeyToString(pHealingOptions.EvtcRpcEnabledHotkey).c_str());

	ImGui::EndGroup();
	ImGuiEx::AddTooltipToLastItem(u8"用於切換即時統計資料共享按鍵的數值\n"
		u8"(虛擬按鍵代碼)");

	ImGuiEx::SmallInputText(u8"EVTC RPC 伺服器", pHealingOptions.EvtcRpcEndpoint, sizeof(pHealingOptions.EvtcRpcEndpoint));
	ImGuiEx::AddTooltipToLastItem(
		u8"用於 EVTC RPC 通訊的伺服器\n"
		u8"(允許其他團隊成員看到你的治療統計數據)。\n"
		u8"所有本機戰鬥事件都會傳送到該伺服器。\n"
		u8"請先確保您信任它。");
	Display_EvtcRpcStatus(pHealingOptions);

	ImGui::Separator();

	if (ImGui::Button(u8"重置所有設定") == true)
	{
		pHealingOptions.Reset();
		LogD("Reset settings");
	}
	ImGuiEx::AddTooltipToLastItem(u8"將所有全域和視窗特定設定重設為其預設值。");
}

void FindAndResolveCyclicDependencies(HealTableOptions& pHealingOptions, size_t pStartIndex)
{
	std::unordered_map<ImGuiID, size_t> windowIdToWindowIndex;
	for (size_t i = 0; i < pHealingOptions.Windows.size(); i++)
	{
		HealWindowContext& window = pHealingOptions.Windows[i];
		if (window.WindowId != 0)
		{
			windowIdToWindowIndex.emplace(window.WindowId, i);
		}
	}

	std::vector<size_t> traversedNodes;
	size_t curIndex = pStartIndex;
	while (pHealingOptions.Windows[curIndex].AnchorWindowId != 0)
	{
		if (std::find(traversedNodes.begin(), traversedNodes.end(), curIndex) != traversedNodes.end())
		{
			traversedNodes.emplace_back(curIndex); // add the node to the list so the printed string is more useful
			std::string loop_string =
				std::accumulate(traversedNodes.begin(), traversedNodes.end(), std::string{},
					[](const std::string& pLeft, const size_t& pRight) -> std::string
					{
						std::string separator = (pLeft.length() > 0 ? " -> " : "");
						return pLeft + separator + std::to_string(pRight);
					});

			LogW("Found cyclic loop {}, removing link from {}", loop_string, pStartIndex);

			pHealingOptions.Windows[pStartIndex].AnchorWindowId = 0;
			return;
		}

		traversedNodes.emplace_back(curIndex);

		auto iter = windowIdToWindowIndex.find(pHealingOptions.Windows[curIndex].AnchorWindowId);
		if (iter == windowIdToWindowIndex.end())
		{
			LogD("Chain starting at {} points to window {} which is not a window we have control over, can't determine if it's cyclic or not",
				pStartIndex, pHealingOptions.Windows[curIndex].AnchorWindowId);
			return;
		}
		curIndex = iter->second;
	}

	LogD("Chain starting at {} ends at 0, not cyclic", pStartIndex);
}

static void RepositionWindows(HealTableOptions& pHealingOptions)
{
	size_t iterations = 0;
	size_t lastChangedPos = 0;

	// Loop windows and try to reposition them. Whenever a window was moved, try to reposition all windows again (to
	// solve possible dependencies between windows). Maximum loop count is equal to the window count, otherwise the
	// windows probably have a circular dependency
	for (; iterations < pHealingOptions.Windows.size(); iterations++)
	{
		for (size_t i = 0; i < pHealingOptions.Windows.size(); i++)
		{
			if (iterations > 0 && i == lastChangedPos)
			{
				LogT("Aborting repositioning on position {} after {} iterations", lastChangedPos, iterations);
				return;
			}

			const HealWindowContext& curWindow = pHealingOptions.Windows[i];
			ImVec2 posVector{static_cast<float>(curWindow.RelativeX), static_cast<float>(curWindow.RelativeY)};

			ImGuiWindow* window = ImGui::FindWindowByID(curWindow.WindowId);
			if (window != nullptr)
			{
				if (ImGuiEx::WindowReposition(
						window,
						curWindow.PositionRule,
						posVector,
						curWindow.RelativeScreenCorner,
						curWindow.AnchorWindowId,
						curWindow.RelativeAnchorWindowCorner,
						curWindow.RelativeSelfCorner) == true)
				{
					LogT("Window {} moved!", i);
					lastChangedPos = i;
				}
			}
		}
	}

	LogW("Aborted repositioning after {} iterations - recursive window relationships?", iterations);
}

void Display_PostNewFrame(ImGuiContext* /*pImguiContext*/, HealTableOptions& pHealingOptions)
{
	// Repositioning windows at the start of the frame prevents lag when moving windows
	RepositionWindows(pHealingOptions);
}

void Display_PreEndFrame(ImGuiContext* pImguiContext, HealTableOptions& pHealingOptions)
{
	float font_size = pImguiContext->FontSize * 2.0f;

	for (size_t i = 0; i < pHealingOptions.AnchoringHighlightedWindows.size(); i++)
	{
		ImGuiWindow* window = ImGui::FindWindowByID(pHealingOptions.AnchoringHighlightedWindows[i]);

		if (window != nullptr)
		{
			std::string text = std::to_string(i);

			ImVec2 regionSize{32.0f, 32.0f};
			ImVec2 regionPos = DrawListEx::CalcCenteredPosition(window->Pos, window->Size, regionSize);

			ImVec2 textSize = pImguiContext->Font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, text.c_str());
			ImVec2 textPos = DrawListEx::CalcCenteredPosition(regionPos, regionSize, textSize);

			window->DrawList->AddRectFilled(regionPos, regionPos + regionSize, IM_COL32(0, 0, 0, 255));
			window->DrawList->AddText(pImguiContext->Font, font_size, textPos, ImGui::GetColorU32(ImGuiCol_Text), text.c_str());
		}
	}
	//
	//RepositionWindows(pHealingOptions);
}

void ImGui_ProcessKeyEvent(HWND /*pWindowHandle*/, UINT pMessage, WPARAM pAdditionalW, LPARAM /*pAdditionalL*/)
{
	ImGuiIO& io = ImGui::GetIO();

	switch (pMessage)
	{
	case WM_KEYUP:
	case WM_SYSKEYUP: // WM_SYSKEYUP is called when a key is pressed with the ALT held down
		io.KeysDown[static_cast<int>(pAdditionalW)] = false;
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN: // WM_SYSKEYDOWN is called when a key is pressed with the ALT held down
		io.KeysDown[static_cast<int>(pAdditionalW)] = true;
		break;

	default:
		break;
	}
}
