#include "imgui.h"
#include "imgui_toggle.h"

namespace ImGui {
	void ToggleButton(const char* label, bool* v) {
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		if (*v) {		
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(204, 233, 252));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(204, 233, 252));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(204, 233, 252));
			ImGui::PushStyleColor(ImGuiCol_Border, (ImVec4)ImColor(0, 122, 204));
			ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(64, 73, 79));

			if (ImGui::Button(label)) {
				*v = !(*v);
			}

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
		}
		else {
			if (ImGui::Button(label)) {
				*v = !(*v);
			}
		}
		ImGui::PopStyleVar();
	}
}