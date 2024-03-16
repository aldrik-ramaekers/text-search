#include "fonts.h"
#include "../fonts/SourceSansProRegular.h"
#include "../fonts/GmarketSans.h"
#include "../fonts/NotoSansJP.h"
#include "../fonts/NotoSerifTC.h"
#include "../fonts/NotoSansSC.h"
#include "../fonts/NotoSansThai.h"
#include "imgui.h"

#include <stdio.h>

ts_font_range ts_locale_to_range(wchar_t* buffer) {
	ts_font_range result = FONT_RANGE_ENGLISH;
	if (wcscmp(buffer, L"el-GR") == 0) result = FONT_RANGE_GREEK;
	if (wcscmp(buffer, L"ko-KR") == 0) result = FONT_RANGE_KOREAN;
	if (wcscmp(buffer, L"ja-JP") == 0) result = FONT_RANGE_JAPANESE;

	if (wcscmp(buffer, L"be-BY") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"bg-BG") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"ru-RU") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"ru-MD") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"ro-MD") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"kk-KZ") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"tt-RU") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"ky-KG") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"mn-MN") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"az-Cyrl-AZ") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"uz-Cyrl-UZ") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"sr-Cyrl-CS") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"sr-Latn-CS") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"az-AZ") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"uz-UZ") == 0) result = FONT_RANGE_CYRILLIC;
	if (wcscmp(buffer, L"sr-CS") == 0) result = FONT_RANGE_CYRILLIC;

	if (wcscmp(buffer, L"bo-CN") == 0) result = FONT_RANGE_CHINESE_SIMPLE;
	if (wcscmp(buffer, L"zh-CN") == 0) result = FONT_RANGE_CHINESE_SIMPLE;
	if (wcscmp(buffer, L"mn-Mong-CN") == 0) result = FONT_RANGE_CHINESE_SIMPLE;
	if (wcscmp(buffer, L"zh-HK") == 0) result = FONT_RANGE_CHINESE_FULL;
	if (wcscmp(buffer, L"zh-TW") == 0) result = FONT_RANGE_CHINESE_FULL;
	if (wcscmp(buffer, L"zh-SG") == 0) result = FONT_RANGE_CHINESE_SIMPLE;
	if (wcscmp(buffer, L"zh-MO") == 0) result = FONT_RANGE_CHINESE_FULL;

	if (wcscmp(buffer, L"th-TH") == 0) result = FONT_RANGE_THAI;
	if (wcscmp(buffer, L"vi-VN") == 0) result = FONT_RANGE_VIETNAMESE;
	return result;
}

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