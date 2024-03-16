#include "fonts.h"
#include "../fonts/SourceSansProRegular.h"
#include "../fonts/GmarketSans.h"
#include "../fonts/NotoSansJP.h"
#include "../fonts/NotoSerifTC.h"
#include "../fonts/NotoSansSC.h"
#include "../fonts/NotoSansThai.h"
#include "imgui.h"

#include <stdio.h>

void ts_load_fonts(float size, ts_font_range locale) {
	ImGuiIO& io = ImGui::GetIO();
	ImFontConfig config;
	config.MergeMode = true;

	static const ImWchar arrow_r[] =
	{
		0x2192, 0x2193, // → character.
		0,
	};

	static const ImWchar triangles[] =
	{
		0x25B6, 0x25BC, // ▶ ▼ characters.
		0,
	};

	// We always use Source Sans Regular for the english, cyrillic and greek alphabet.
	// other languages will be displayed in their respective font.

	ImFontGlyphRangesBuilder builder;
	ImVector<ImWchar> ranges;
	builder.AddRanges(arrow_r);
	builder.AddRanges(triangles);

	builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
	if (locale == FONT_RANGE_GREEK) 
		builder.AddRanges(io.Fonts->GetGlyphRangesGreek());

	if (locale == FONT_RANGE_CYRILLIC)
		builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());

	builder.BuildRanges(&ranges);
	
	ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(
		SourceSansProRegular_compressed_data, 
		SourceSansProRegular_compressed_size, 
		size, nullptr, ranges.Data);

	IM_ASSERT(font != nullptr);
	io.FontDefault = font;
				
	// Fonts can be found in fonts/ folder.
	if (locale == FONT_RANGE_KOREAN) {
		size /= 1.5f;
		io.Fonts->AddFontFromMemoryCompressedTTF(
				GmarketSans_compressed_data, 
				GmarketSans_compressed_size, 
				size, &config, io.Fonts->GetGlyphRangesKorean());
	}
	else if (locale == FONT_RANGE_JAPANESE) {
		io.Fonts->AddFontFromMemoryCompressedTTF(
				NotoSansJP_compressed_data, 
				NotoSansJP_compressed_size, 
				size, &config, io.Fonts->GetGlyphRangesJapanese());
	}
	else if (locale == FONT_RANGE_CHINESE_FULL) {
		io.Fonts->AddFontFromMemoryCompressedTTF(
				NotoSerifTC_compressed_data, 
				NotoSerifTC_compressed_size, 
				size, &config, io.Fonts->GetGlyphRangesChineseFull());
	}
	else if (locale == FONT_RANGE_CHINESE_SIMPLE) {
		io.Fonts->AddFontFromMemoryCompressedTTF(
				NotoSansSC_compressed_data, 
				NotoSansSC_compressed_size, 
				size, &config, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
	}
	else if (locale == FONT_RANGE_THAI) {
		io.Fonts->AddFontFromMemoryCompressedTTF(
				NotoSansThai_compressed_data, 
				NotoSansThai_compressed_size, 
				size, &config, io.Fonts->GetGlyphRangesThai());
	}
	else if (locale == FONT_RANGE_VIETNAMESE) {
		io.Fonts->AddFontFromMemoryCompressedTTF(
				NotoSansJP_compressed_data, 
				NotoSansJP_compressed_size, 
				size, &config, io.Fonts->GetGlyphRangesVietnamese());
	}

	io.Fonts->Build();    
}