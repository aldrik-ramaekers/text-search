#include "imgui_spectrum.h"
#include "../fonts/GmarketSans.h"
#include "../fonts/NotoSansJP.h"
#include "../fonts/SourceSansProRegular.h"
#include "imgui.h"

namespace ImGui {
    namespace Spectrum {
        void LoadFont(float size) {
            ImGuiIO& io = ImGui::GetIO();
			ImFontConfig config;
			config.MergeMode = true;
			
			{
				ImFontGlyphRangesBuilder builder;
				ImVector<ImWchar> ranges;
				builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
				builder.AddRanges(io.Fonts->GetGlyphRangesGreek());
				builder.BuildRanges(&ranges);

				ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(
					SourceSansProRegular_compressed_data, 
					SourceSansProRegular_compressed_size, 
					size, nullptr, ranges.Data);

				IM_ASSERT(font != nullptr);
				io.FontDefault = font;
				
			}
			
			// Uncomment if you want these glyphs. Fonts can be found in fonts/ folder.
			// io.Fonts->AddFontFromMemoryCompressedTTF(
			// 		GmarketSans_compressed_data, 
			// 		GmarketSans_compressed_size, 
			// 		size, &config, io.Fonts->GetGlyphRangesKorean());

			// io.Fonts->AddFontFromMemoryCompressedTTF(
			// 		NotoSansJP_compressed_data, 
			// 		NotoSansJP_compressed_size, 
			// 		size, &config, io.Fonts->GetGlyphRangesJapanese());
			

            io.Fonts->Build();          
        }

        void StyleColorsSpectrum() {
            ImGuiStyle* style = &ImGui::GetStyle();
            style->GrabRounding = 4.0f;
			style->TabRounding = 4.f;
			style->FrameRounding = 4.f;

            ImVec4* colors = style->Colors;
            colors[ImGuiCol_Text] = ColorConvertU32ToFloat4(Spectrum::GRAY800); // text on hovered controls is gray900
            colors[ImGuiCol_TextDisabled] = ColorConvertU32ToFloat4(Spectrum::GRAY500);
            colors[ImGuiCol_WindowBg] = ColorConvertU32ToFloat4(Spectrum::GRAY100);
            colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_PopupBg] = ColorConvertU32ToFloat4(Spectrum::GRAY50); // not sure about this. Note: applies to tooltips too.
            colors[ImGuiCol_BorderShadow] = ColorConvertU32ToFloat4(Spectrum::Static::NONE); // We don't want shadows. Ever.
            colors[ImGuiCol_FrameBg] = ColorConvertU32ToFloat4(Spectrum::GRAY75); // this isnt right, spectrum does not do this, but it's a good fallback
            colors[ImGuiCol_FrameBgHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY50);
            colors[ImGuiCol_FrameBgActive] = ColorConvertU32ToFloat4(Spectrum::GRAY200);
            colors[ImGuiCol_TitleBg] = ColorConvertU32ToFloat4(Spectrum::GRAY300); // those titlebar values are totally made up, spectrum does not have this.
            colors[ImGuiCol_TitleBgActive] = ColorConvertU32ToFloat4(Spectrum::GRAY200);
            colors[ImGuiCol_TitleBgCollapsed] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
            colors[ImGuiCol_ScrollbarBg] = ColorConvertU32ToFloat4(Spectrum::GRAY100); // same as regular background
            colors[ImGuiCol_ScrollbarGrab] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
            colors[ImGuiCol_ScrollbarGrabHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY600);
            colors[ImGuiCol_ScrollbarGrabActive] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
            colors[ImGuiCol_CheckMark] = ColorConvertU32ToFloat4(Spectrum::BLUE500);
            colors[ImGuiCol_SliderGrab] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
            colors[ImGuiCol_SliderGrabActive] = ColorConvertU32ToFloat4(Spectrum::GRAY800);
            colors[ImGuiCol_Button] = ColorConvertU32ToFloat4(Spectrum::GRAY75); // match default button to Spectrum's 'Action Button'.
            colors[ImGuiCol_ButtonHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY50);
            colors[ImGuiCol_ButtonActive] = ColorConvertU32ToFloat4(Spectrum::GRAY200);
            colors[ImGuiCol_Separator] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
            colors[ImGuiCol_SeparatorHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY600);
            colors[ImGuiCol_SeparatorActive] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
            colors[ImGuiCol_ResizeGrip] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
            colors[ImGuiCol_ResizeGripHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY600);
            colors[ImGuiCol_ResizeGripActive] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
            colors[ImGuiCol_PlotLines] = ColorConvertU32ToFloat4(Spectrum::BLUE400);
            colors[ImGuiCol_PlotLinesHovered] = ColorConvertU32ToFloat4(Spectrum::BLUE600);
            colors[ImGuiCol_PlotHistogram] = ColorConvertU32ToFloat4(Spectrum::BLUE400);
            colors[ImGuiCol_PlotHistogramHovered] = ColorConvertU32ToFloat4(Spectrum::BLUE600);
            colors[ImGuiCol_TextSelectedBg] = ColorConvertU32ToFloat4((Spectrum::BLUE400 & 0x00FFFFFF) | 0x33000000);
            colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
            colors[ImGuiCol_NavHighlight] = ColorConvertU32ToFloat4((Spectrum::GRAY900 & 0x00FFFFFF) | 0x0A000000);
            colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
            colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
            colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

		    colors[ImGuiCol_Border] = ColorConvertU32ToFloat4(Color(0xCCCCCC));
			colors[ImGuiCol_MenuBarBg] = ColorConvertU32ToFloat4(Spectrum::GRAY300);
			colors[ImGuiCol_Header] = ColorConvertU32ToFloat4(Spectrum::GRAY300);
            colors[ImGuiCol_HeaderHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY200);
            colors[ImGuiCol_HeaderActive] = ColorConvertU32ToFloat4(Spectrum::GRAY200);
			colors[ImGuiCol_TableBorderStrong] = ColorConvertU32ToFloat4(Color(0xE7E7E7));
			colors[ImGuiCol_TableBorderLight] = ColorConvertU32ToFloat4(Color(0xE7E7E7));
			colors[ImGuiCol_TableHeaderBg] = ColorConvertU32ToFloat4(Spectrum::GRAY300);
			colors[ImGuiCol_TableRowBg] = ColorConvertU32ToFloat4(Color(0xFFFFFF));
			colors[ImGuiCol_TableRowBgAlt] = ColorConvertU32ToFloat4(Spectrum::GRAY100);
        }

        

    }
}
