#pragma once
#include "imgui.h"

#include <optional>

// Helpers for ImGui functions
namespace ImGuiEx
{
	template <typename... Args>
	void TextRightAlignedSameLine(const char* pFormatString, Args... pArgs)
	{
		char buffer[1024];

		snprintf(buffer, sizeof(buffer), pFormatString, pArgs...);
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(buffer).x); // Sending x in SameLine messes with alignment when inside of a group
		ImGui::Text("%s", buffer);
	}

	template <typename... Args>
	void TextColoredCentered(ImColor pColor, const char* pFormatString, Args... pArgs)
	{
		char buffer[1024];

		snprintf(buffer, sizeof(buffer), pFormatString, pArgs...);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x * 0.5f - ImGui::CalcTextSize(buffer).x * 0.5f);
		ImGui::TextColored(pColor, "%s", buffer);
	}

	template <typename... Args>
	void BottomText(const char* pFormatString, Args... pArgs)
	{
		char buffer[1024];

		snprintf(buffer, sizeof(buffer), pFormatString, pArgs...);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y - ImGui::CalcTextSize(buffer).y);
		ImGui::Text("%s", buffer);
	}

	template <typename... Args>
	void AddTooltipToLastItem(const char* pFormatString, Args... pArgs)
	{
		if (ImGui::IsItemHovered() == true)
		{
			ImGui::SetTooltip(pFormatString, pArgs...);
		}
	}

	void StatsEntry(const char* pLeftText, const char* pRightText, std::optional<float> pFillRatio);
}