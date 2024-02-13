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
	u8"����", u8"�}�� (�ȤU��í�w����)", u8"�}�� (�U������/í�w����)"
};
static constexpr EnumStringArray<DataSource> DATA_SOURCE_ITEMS{
	u8"�ؼ�", u8"�ޯ�", u8"�`�p", u8"��X", u8"�έ���X�v��"};
static constexpr EnumStringArray<SortOrder> SORT_ORDER_ITEMS{
	u8"���r����A��Z(�ɧ�)", u8"���r����Z��A(����)", u8"���C��v���q�Ѥp��j(�ɧ�)", u8"���C��v���q�Ѥj��p(����)"};
static constexpr EnumStringArray<CombatEndCondition> COMBAT_END_CONDITION_ITEMS{
	u8"���}�԰�", u8"�̫�ˮ`�ƥ�", u8"�̫�v���ƥ�", u8"�̫�ˮ`/�v���ƥ�"};
static constexpr EnumStringArray<spdlog::level::level_enum, 7> LOG_LEVEL_ITEMS{
	u8"�l��", u8"����", u8"��T", u8"ĵ�i", u8"���~", u8"�P�R", u8"����"};

static constexpr EnumStringArray<Position, static_cast<size_t>(Position::WindowRelative) + 1> POSITION_ITEMS{
	u8"���", u8"�ù��۹��m", u8"�����۹��m"};
static constexpr EnumStringArray<CornerPosition, static_cast<size_t>(CornerPosition::BottomRight) + 1> CORNER_POSITION_ITEMS{
	u8"���W", u8"�k�W", u8"���U", u8"�k�U"};

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
		ImGui::InputText(u8"���خ榡", pContext.DetailsEntryFormat, sizeof(pContext.DetailsEntryFormat));
		ImGuiEx::AddTooltipToLastItem(u8"��ܸ�ƪ��榡(�έp��Ƭ��C�ӱ���)�C\n"
		                              u8"{1}: �v���q\n"
		                              u8"{2}: �R����\n"
		                              u8"{3}: �I�k�� (�|����@)\n"
		                              u8"{4}: �C��v���q\n"
		                              u8"{5}: �C���R���v���q\n"
		                              u8"{6}: �C���I�k�v���q (�|����@)\n"
		                              u8"{7}: ���`�v���q�ʤ���\n");

		ImGui::EndPopup();
	}

	ImVec4 bgColor = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
	bgColor.w = 0.0f;
	ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
	snprintf(buffer, sizeof(buffer), "##HEALDETAILS.TOTALS.%i.%llu", static_cast<int>(pDataSource), pState.Id);
	ImGui::BeginChild(buffer, ImVec2(ImGui::GetWindowContentRegionWidth() * 0.35f, 0));
	ImGui::Text(u8"�`�v���q");
	ImGuiEx::TextRightAlignedSameLine("%llu", pState.Healing);

	ImGui::Text(u8"�R����");
	ImGuiEx::TextRightAlignedSameLine("%llu", pState.Hits);

	if (pState.Casts.has_value() == true)
	{
		ImGui::Text(u8"�I�k��");
		ImGuiEx::TextRightAlignedSameLine("%llu", *pState.Casts);
	}

	ImGui::Text(u8"�C��v���q");
	ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(pState.Healing, pState.TimeInCombat));

	ImGui::Text(u8"�C���R���v���q");
	ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(pState.Healing, pState.Hits));

	if (pState.Casts.has_value() == true)
	{
		ImGui::Text(u8"�C���I�k�v���q");
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
			ImGui::TextWrapped(u8"�Y�ɲέp��Ʀ@�ɤw���ΡC �ҥ�\"�v���έp�ﶵ\"����\"�Y�ɲέp��Ʀ@��\"�A�H�K�d�ݹζ�����L���a���v�����p�C");
			pContext.CurrentFrameLineCount += 3;
			return;
		}

		evtc_rpc_client_status status = GlobalObjects::EVTC_RPC_CLIENT->GetStatus();
		if (status.Connected == false)
		{
			ImGui::TextWrapped(u8"���s����Y�ɲέp�@�ɦ��A�� (\"%s\")�C", status.Endpoint.c_str());
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
			ImGui::TextUnformatted(u8"�۹��ù�����");
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
			ImGui::TextUnformatted(u8"�q���I���O����");
			ImGuiEx::SmallIndent();
			ImGuiEx::SmallEnumRadioButton("AnchorCornerPositionEnum", pContext.RelativeAnchorWindowCorner, CORNER_POSITION_ITEMS);
			ImGuiEx::SmallUnindent();

			ImGui::TextUnformatted(u8"�즹���O����");
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
			if (ImGui::BeginCombo(u8"���I����", selectedWindowName) == true)
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
		ImGuiEx::ComboMenu(u8"��ƨӷ�", pContext.DataSourceChoice, DATA_SOURCE_ITEMS);
		ImGuiEx::AddTooltipToLastItem(u8"�M�w��������ܭ��Ǹ��");

		if (pContext.DataSourceChoice != DataSource::Totals)
		{
			ImGuiEx::ComboMenu(u8"�Ƨ�", pContext.SortOrderChoice, SORT_ORDER_ITEMS);
			ImGuiEx::AddTooltipToLastItem(u8"�M�w�p��b\"�ؼ�\"�M\"�ޯ�\"��������ؼЩM�ޯ�i��ƧǡC");

			if (ImGui::BeginMenu(u8"�έp��Ʊư�") == true)
			{
				ImGuiEx::SmallCheckBox(u8"�p��", &pContext.ExcludeGroup);
				ImGuiEx::SmallCheckBox(u8"�p���~", &pContext.ExcludeOffGroup);
				ImGuiEx::SmallCheckBox(u8"�ζ��~", &pContext.ExcludeOffSquad);
				ImGuiEx::SmallCheckBox(u8"�l�ꪫ", &pContext.ExcludeMinions);
				ImGuiEx::SmallCheckBox(u8"��L", &pContext.ExcludeUnmapped);

				ImGui::EndMenu();
			}
		}

		ImGuiEx::ComboMenu(u8"�԰�����", pContext.CombatEndConditionChoice, COMBAT_END_CONDITION_ITEMS);
		ImGuiEx::AddTooltipToLastItem(u8"�M�w���ӨϥμзǨӽT�w�԰�����\n"
										u8"(�H�ξ԰������ɶ�)");

		ImGuiEx::SmallUnindent();
		ImGui::Separator();

		if (ImGui::BeginMenu(u8"���") == true)
		{
			ImGuiEx::SmallCheckBox(u8"��ܱ��ι�", &pContext.ShowProgressBars);
			ImGuiEx::AddTooltipToLastItem(u8"�b�C�ӱ��ؤU����ܤ@�Ӧ�����A��ܸӱ��ػP�̤j���ت����");

			ImGui::SetNextItemWidth(260.0f);
			ImGuiEx::SmallInputText(u8"²��", pContext.Name, sizeof(pContext.Name));
			ImGuiEx::AddTooltipToLastItem(u8"�Ω�b\"�v���έp\"��椤��ܦ��������W��");

			ImGui::SetNextItemWidth(39.0f);
			ImGuiEx::SmallInputInt(u8"�̤j�W�r����", &pContext.MaxNameLength);
			ImGuiEx::AddTooltipToLastItem(
				u8"�N��ܪ��W�ٺI�_���ҳ]�w���ת��r��C �]�w�� 0 �H���ΡC");

			ImGui::SetNextItemWidth(39.0f);
			ImGuiEx::SmallInputInt(u8"�̤p��ܦ��", &pContext.MinLinesDisplayed);
			ImGuiEx::AddTooltipToLastItem(
				u8"����������ܪ��̤p��Ʀ�ơC");

			ImGui::SetNextItemWidth(39.0f);
			ImGuiEx::SmallInputInt(u8"�̤j��ܦ��", &pContext.MaxLinesDisplayed);
			ImGuiEx::AddTooltipToLastItem(
				u8"����������ܪ��̤j��Ʀ�ơC �]�w�� 0 ��ܵL����");

			ImGui::SetNextItemWidth(260.0f);
			ImGuiEx::SmallInputText(u8"�έp��Ʈ榡", pContext.EntryFormat, sizeof(pContext.EntryFormat));
			if (pContext.DataSourceChoice != DataSource::Totals)
			{
				ImGuiEx::AddTooltipToLastItem(u8"��ܸ�ƪ��榡(�έp��Ƭ��C�ӱ���)�C\n"
					u8"{1}: �v���q\n"
					u8"{2}: �R����\n"
					u8"{3}: �I�k�� (�|����@)\n"
					u8"{4}: �C��v���q\n"
					u8"{5}: �C���R���v���q\n"
					u8"{6}: �C���I�k�v���q (�|����@)\n"
					u8"{7}: ���`�v���q�ʤ���");
			}
			else
			{
				ImGuiEx::AddTooltipToLastItem(u8"��ܸ�ƪ��榡(�έp��Ƭ��C�ӱ���)�C\n"
					u8"{1}: �v���q\n"
					u8"{2}: �R����\n"
					u8"{3}: �I�k�� (�|����@)\n"
					u8"{4}: �C��v���q\n"
					u8"{5}: �C���R���v���q\n"
					u8"{6}: �C���I�k�v���q (�|����@)");
			}

			ImGui::SetNextItemWidth(260.0f);
			ImGuiEx::SmallInputText(u8"���D��榡", pContext.TitleFormat, sizeof(pContext.TitleFormat));
			if (pContext.DataSourceChoice != DataSource::Totals)
			{
				ImGuiEx::AddTooltipToLastItem(u8"���������D���榡�C\n"
					u8"{1}: �`�v���q\n"
					u8"{2}: �`�R����\n"
					u8"{3}: �`�I�k�� (�|����@)\n"
					u8"{4}: �C��v���q\n"
					u8"{5}: �C���R���v���q\n"
					u8"{6}: �C���I�k�v���q (�|����@)\n"
					u8"{7}: �԰��ɶ�");
			}
			else
			{
				ImGuiEx::AddTooltipToLastItem(u8"���������D���榡�C\n"
					u8"{1}: �԰��ɶ�");
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(u8"�˦�") == true)
		{
			ImGuiEx::SmallEnumCheckBox(u8"���D��", &pContext.WindowFlags, ImGuiWindowFlags_NoTitleBar, true);
			ImGuiEx::SmallEnumCheckBox(u8"�u�ʱ�", &pContext.WindowFlags, ImGuiWindowFlags_NoScrollbar, true);
			ImGuiEx::SmallEnumCheckBox(u8"�I��", &pContext.WindowFlags, ImGuiWindowFlags_NoBackground, true);
			
			ImGui::Separator();

			ImGuiEx::SmallCheckBox(u8"�۰ʽվ�����j�p", &pContext.AutoResize);
			if (pContext.AutoResize == false)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(128, 128, 128, 255));
			}
			ImGuiEx::SmallIndent();

			ImGui::SetNextItemWidth(65.0f);
			ImGuiEx::SmallInputInt(u8"�����e��", &pContext.FixedWindowWidth);
			ImGuiEx::AddTooltipToLastItem(
				u8"�]�w�� 0 �H�ʺA�վ�e�פj�p");

			ImGuiEx::SmallUnindent();
			if (pContext.AutoResize == false)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleColor();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(u8"��m�y��") == true)
		{
			Display_WindowOptions_Position(pHealingOptions, pContext);
			ImGui::EndMenu();
		}

		ImGui::Separator();

		float oldPosY = ImGui::GetCursorPosY();
		ImGui::BeginGroup();

		ImGui::SetCursorPosY(oldPosY + ImGui::GetStyle().FramePadding.y);
		ImGui::Text(u8"����");

		ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
		ImGui::SetCursorPosY(oldPosY);

		ImGui::SetNextItemWidth(ImGui::CalcTextSize("-----").x);
		ImGui::InputInt("##HOTKEY", &pContext.Hotkey, 0);

		ImGui::SameLine();
		ImGui::Text("(%s)", VirtualKeyToString(pContext.Hotkey).c_str());

		ImGui::EndGroup();
		ImGuiEx::AddTooltipToLastItem(u8"�Ω�}�ҩM�������������䪺�ƭ�\n"
										u8"(��������N�X)");

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
			ImGuiEx::TextColoredCentered(ImColor(0, 209, 165), u8"�`�p");
			Display_Content(curWindow, DataSource::Totals, i, pHealingOptions.EvtcRpcEnabled);
			ImGui::PopID();

			ImGui::PushID(static_cast<int>(DataSource::Agents));
			ImGuiEx::TextColoredCentered(ImColor(0, 209, 165), u8"�ؼ�");
			Display_Content(curWindow, DataSource::Agents, i, pHealingOptions.EvtcRpcEnabled);
			ImGui::PopID();

			ImGui::PushID(static_cast<int>(DataSource::Skills));
			ImGuiEx::TextColoredCentered(ImColor(0, 209, 165), u8"�ޯ�");
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
		ImGui::TextColored(ImVec4(0.0f, 0.75f, 0.0f, 1.0f), u8"�w�s�u�� %s %llu ��", status.Endpoint.c_str(), seconds);
	}
	else if (pHealingOptions.EvtcRpcEnabled == false)
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), u8"�ѩ󥼱ҥΧY�ɲέp��Ʀ@�ɦӥ��i��s��");
	}
	else
	{
		ImGui::TextColored(ImVec4(0.75f, 0.0f, 0.0f, 1.0f), u8"�L�k�s�u�� %s�C ���դ�...", status.Endpoint.c_str());
	}
}

void Display_AddonOptions(HealTableOptions& pHealingOptions)
{
	ImGui::TextUnformatted(u8"�v���έp");
	ImGuiEx::SmallIndent();
	for (uint32_t i = 0; i < HEAL_WINDOW_COUNT; i++)
	{
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "(%u) %s", i, pHealingOptions.Windows[i].Name);
		ImGuiEx::SmallCheckBox(buffer, &pHealingOptions.Windows[i].Shown);
	}
	ImGuiEx::SmallUnindent();

	ImGuiEx::ComboMenu(u8"�۰ʧ�s", pHealingOptions.AutoUpdateSetting, AUTO_UPDATE_SETTING_ITEMS);
	ImGuiEx::SmallCheckBox(u8"�����Ҧ�", &pHealingOptions.DebugMode);
	ImGuiEx::AddTooltipToLastItem(
		u8"�b�ؼЩM�ޯ�W�٤��]�A������ơC\n"
		u8"�b�^����b�p����D�I�ϫe�A���}���ﶵ�C");

	spdlog::string_view_t log_level_names[] = SPDLOG_LEVEL_NAMES;
	std::string log_level_items[std::size(log_level_names)];
	for (size_t i = 0; i < std::size(log_level_names); i++)
	{
		log_level_items[i] = std::string_view(log_level_names[i].data(), log_level_names[i].size());
	}

	if (ImGuiEx::ComboMenu(u8"��������", pHealingOptions.LogLevel, LOG_LEVEL_ITEMS) == true)
	{
		Log_::SetLevel(pHealingOptions.LogLevel);
	}
	ImGuiEx::AddTooltipToLastItem(
		u8"�p�G���]�w�������A�N�}�үS�w�����Y���ʼh�Ū���x�����C\n"
		u8"��x�O�s�b addons\\logs\\arcdps_healing_stats\\�C\n"
		u8"��x�O���N��į�y���@�I�v�T�C");

	ImGui::Separator();

	if (ImGuiEx::SmallCheckBox(u8"�N�v�������� EVTC ��x��", &pHealingOptions.EvtcLoggingEnabled) == true)
	{
		GlobalObjects::EVENT_PROCESSOR->SetEvtcLoggingEnabled(pHealingOptions.EvtcLoggingEnabled);
	}

	if (ImGuiEx::SmallCheckBox(u8"�ҥΧY�ɲέp��Ʀ@��", &pHealingOptions.EvtcRpcEnabled) == true)
	{
		GlobalObjects::EVTC_RPC_CLIENT->SetEnabledStatus(pHealingOptions.EvtcRpcEnabled);
	}
	ImGuiEx::AddTooltipToLastItem(
		u8"���\�P�p��������L���a�Y�ɦ@�ɪv���έp��ơC\n"
		u8"�o�O�z�L�������A��������(�аѾ\�U�����ﶵ)\n"
		u8"�ҥΧY�ɦ@�ɫ�A�b���a�ϥH�K���T�˴���Ҧ�\n"
		u8"�p���������e�A���i��L�k���`�u�@\n"
		u8"\n"
		u8"�i�z�L�v���έp�����d�ݨ�L���a���ɪ��έp���\n"
		u8"�A���ƨӷ��]�w�� \"%s\"\n"
		u8"\n"
		u8"�ҥΦ��ﶵ�N��į�y���@�I�v�T�A�åB�|��z��\n"
		u8"�����s�u�y���@���B�~�t��(�̦h�� 10 kiB/s �W��\n"
		u8"�M 100 kiB/s �U��)�C\n"
		u8"�ζ����ҥΧY�ɲέp��Ʀ@�ɪ����a�V�h�A�U���s�u\n"
		u8"�ϥζq�N�|�W�[�A�ӤW�ǳs�u�ϥζq�h���|�C"
		, DATA_SOURCE_ITEMS[DataSource::PeersOutgoing]);

	if (pHealingOptions.EvtcRpcEnabled == false)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(128, 128, 128, 255));
	}
	if (ImGuiEx::SmallCheckBox(u8"�Y�ɲέp��Ʀ@�� �`�ټҦ�", &pHealingOptions.EvtcRpcBudgetMode) == true)
	{
		GlobalObjects::EVTC_RPC_CLIENT->SetBudgetMode(pHealingOptions.EvtcRpcBudgetMode);
	}
	if (pHealingOptions.EvtcRpcEnabled == false)
	{
		ImGui::PopItemFlag();
		ImGui::PopStyleColor();
	}
	ImGuiEx::AddTooltipToLastItem(
		u8"�u�V�έ��o�e�ƥ󪺳̤p�l���C �o��֤F������\n"
		u8"�ϥΪ��W���W�e�q�C ��ܪ��v���έp��Ƥ��M����\n"
		u8"�ǽT�A���O��L���a�b�԰����ݨ쪺�԰��ɶ��i��\n"
		u8"�|�y�L���ǽT�C �p�G�o�Ǫ��a�ϥΪ��O�b�ޤJ�`��\n"
		u8"�Ҧ��䴩���e�o�������󪩥��A�h�Y�Ϧb�D�԰����A�U\n"
		u8"�A�԰��ɶ��]�i��D�`���ǽT�C ���ﶵ��U���W�e\n"
		u8"�ϥζq�S���v�T�A�u�v�T�W�ǡC �ҥΦ��ﶵ��A\n"
		u8"�w���s�u�ϥζq������ < 1 kiB/s �W�ǡC");

	float oldPosY = ImGui::GetCursorPosY();
	ImGui::BeginGroup();

	ImGui::SetCursorPosY(oldPosY + ImGui::GetStyle().FramePadding.y);
	ImGui::Text(u8"�Y�ɲέp��Ʀ@�� ����");

	ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::SetCursorPosY(oldPosY);

	ImGui::SetNextItemWidth(ImGui::CalcTextSize("-----").x);
	ImGui::InputInt("##HOTKEY", &pHealingOptions.EvtcRpcEnabledHotkey, 0);

	ImGui::SameLine();
	ImGui::Text("(%s)", VirtualKeyToString(pHealingOptions.EvtcRpcEnabledHotkey).c_str());

	ImGui::EndGroup();
	ImGuiEx::AddTooltipToLastItem(u8"�Ω�����Y�ɲέp��Ʀ@�ɫ��䪺�ƭ�\n"
		u8"(��������N�X)");

	ImGuiEx::SmallInputText(u8"EVTC RPC ���A��", pHealingOptions.EvtcRpcEndpoint, sizeof(pHealingOptions.EvtcRpcEndpoint));
	ImGuiEx::AddTooltipToLastItem(
		u8"�Ω� EVTC RPC �q�T�����A��\n"
		u8"(���\��L�ζ������ݨ�A���v���έp�ƾ�)�C\n"
		u8"�Ҧ������԰��ƥ󳣷|�ǰe��Ӧ��A���C\n"
		u8"�Х��T�O�z�H�����C");
	Display_EvtcRpcStatus(pHealingOptions);

	ImGui::Separator();

	if (ImGui::Button(u8"���m�Ҧ��]�w") == true)
	{
		pHealingOptions.Reset();
		LogD("Reset settings");
	}
	ImGuiEx::AddTooltipToLastItem(u8"�N�Ҧ�����M�����S�w�]�w���]����w�]�ȡC");
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
