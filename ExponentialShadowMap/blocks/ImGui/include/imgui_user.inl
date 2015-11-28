/*
 Cinder-ImGui
 This code is intended for use with Cinder
 and Omar Cornut ImGui C++ libraries.
 
 http://libcinder.org
 https://github.com/ocornut
 
 Copyright (c) 2013-2015, Simon Geilfus - All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and
 the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include "CinderImGui.h"

using namespace std;
using namespace ci;

bool ImGui::IconButton( const std::string &icon, const ImVec2& size, ImFont* font, bool frame )
{
	ImGuiState& g = *GImGui;
	
	// the icon font should be loaded as the second font for this to work
	if( font == nullptr && ImGui::GetIO().Fonts->Fonts[1] != nullptr ){
		font = ImGui::GetIO().Fonts->Fonts[1];
	}

	// no icon font found
	assert( font != nullptr );
	
	// change font
	ScopedFont scopedFont( font );
	
	const ImVec2& size_arg = size;
	ImGuiButtonFlags flags = 0;
	
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;
	
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(icon.c_str());
	const ImVec2 icon_size = CalcTextSize(icon.c_str(), NULL, true);
	
	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrentLineTextBaseOffset)
		pos.y += window->DC.CurrentLineTextBaseOffset - style.FramePadding.y;
	const ImVec2 bbSize(size_arg.x != 0.0f ? size_arg.x : (icon_size.x + style.FramePadding.x*2), size_arg.y != 0.0f ? size_arg.y : ( icon_size.y + style.FramePadding.y*2));
	
	const ImRect bb(pos, pos + bbSize);
	ItemSize(bb, style.FramePadding.y);
	if (!ItemAdd(bb, &id))
		return false;
	
	if (window->DC.ButtonRepeat) flags |= ImGuiButtonFlags_Repeat;
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, true, flags);
	
	// Render
	const ImU32 col = window->Color((hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	
	if( frame ){
		RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);
		RenderTextClipped(bb.Min, bb.Max, icon.c_str(), NULL, &icon_size, ImGuiAlign_Center | ImGuiAlign_VCenter);
	}
	else {
		ScopedStyleColor color( ImGuiCol_Text, ImVec4( (float)((col >> 0) & 0xFF) / 255.0f, (float)((col >> 8) & 0xFF) / 255.0f, (float)((col >> 16) & 0xFF) / 255.0f, (float)((col >> 24) & 0xFF) / 255.0f ) );
		RenderTextClipped(bb.Min, bb.Max, icon.c_str(), NULL, &icon_size, ImGuiAlign_Center | ImGuiAlign_VCenter);
	}
	
	return pressed;
}
bool ImGui::IconButton( const std::string &icon, const std::string &label, const ImVec2& size, ImFont* font, bool frame )
{
	const ImVec2& size_arg = size;
	ImGuiButtonFlags flags = 0;
	
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;
	
	ImGuiState& g = *GImGui;
	
	// the icon font should be loaded as the second font for this to work
	if( font == nullptr ){
		font = g.IO.Fonts->Fonts[1];
	}
	
	// no icon font found
	assert( font != nullptr );
	
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label.c_str());
	const ImVec2 label_size = CalcTextSize(label.c_str(), NULL, true);
	const ImVec2 icon_size = CalcTextSize(icon.c_str(), NULL, true);
	
	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrentLineTextBaseOffset)
		pos.y += window->DC.CurrentLineTextBaseOffset - style.FramePadding.y;
	const ImVec2 bbSize(size_arg.x != 0.0f ? size_arg.x : (label_size.x + icon_size.x + style.FramePadding.x*6), size_arg.y != 0.0f ? size_arg.y : ( ImMax( icon_size.y, label_size.y ) + style.FramePadding.y*2));
	
	const ImRect bb(pos, pos + bbSize);
	ItemSize(bb, style.FramePadding.y);
	if (!ItemAdd(bb, &id))
		return false;
	
	if (window->DC.ButtonRepeat) flags |= ImGuiButtonFlags_Repeat;
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, true, flags);
	
	// Render
	const ImU32 col = window->Color((hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	
	if( frame ){
		RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);
		{
			// change font
			ScopedFont scopedFont( font );
			
			const ImVec2 iconBbSize( icon_size.x + style.FramePadding.x*4, icon_size.y + style.FramePadding.y*2 );
			const ImRect iconBb( pos, pos + iconBbSize);
			RenderTextClipped(ImVec2(style.FramePadding.x,0)+iconBb.Min, iconBb.Max, icon.c_str(), NULL, &label_size, ImGuiAlign_Center | ImGuiAlign_VCenter);
		}
		RenderTextClipped(ImVec2(style.FramePadding.x*2+icon_size.x,0)+bb.Min, bb.Max, label.c_str(), NULL, &label_size, ImGuiAlign_Center | ImGuiAlign_VCenter);
	}
	else {
		{
			// change font
			ScopedFont scopedFont( font );
			
			ScopedStyleColor color( ImGuiCol_Text, ImVec4( (float)((col >> 0) & 0xFF) / 255.0f, (float)((col >> 8) & 0xFF) / 255.0f, (float)((col >> 16) & 0xFF) / 255.0f, (float)((col >> 24) & 0xFF) / 255.0f ) );
			const ImVec2 iconBbSize( icon_size.x + style.FramePadding.x*4, icon_size.y + style.FramePadding.y*2 );
			const ImRect iconBb( pos, pos + iconBbSize);
			RenderTextClipped(ImVec2(style.FramePadding.x,0)+iconBb.Min, iconBb.Max, icon.c_str(), NULL, &label_size, ImGuiAlign_Center | ImGuiAlign_VCenter);
		}
		RenderTextClipped(ImVec2(style.FramePadding.x*2+icon_size.x,0)+bb.Min, bb.Max, label.c_str(), NULL, &label_size, ImGuiAlign_Center | ImGuiAlign_VCenter);
	}
	
	return pressed;
}

bool ImGui::ColorPicker( const char* label, float col[3] )
{
	// convert colors
	vec3 hsv;
	ImVec4 rgba( col[0], col[1], col[2], 1.0f );
	ColorConvertRGBtoHSV( col[0], col[1], col[2], hsv.x, hsv.y, hsv.z );
	//DragFloat3( string( "##" + string( label ) + "_DragFloat" ).c_str(), &col[0] );
	//SameLine();
	if( ColorButton( rgba ) ){
		OpenPopup( "ColorPickerPopup" );
	}
	SameLine();
	Text( label );
	
	// update textures if needed
	vec2 texSize( 64 );
	bool changes = false;
	static bool needTexUpdate = true;
	static gl::Texture2dRef svTex, hueTex;
	if( needTexUpdate ) {
		Surface svSurface( texSize.x, texSize.y, false );
		for( float x = 0; x < texSize.x; ++x ){
			for( float y = 0; y < texSize.y; ++y ){
				ColorA color( ColorModel::CM_HSV, hsv.x, x / texSize.x, 1.0f - y / texSize.y );
				svSurface.setPixel( ivec2( x, y ), color );
			}
		}
		svTex = gl::Texture2d::create( svSurface );
		
		needTexUpdate = false;
	}
	if( !hueTex ) {
		Surface hueSurface( 10, texSize.y, false );
		for( float x = 0; x < 10; ++x ){
			for( float y = 0; y < texSize.y; ++y ){
				ColorA color( ColorModel::CM_HSV, y / texSize.y, 1.0f, 1.0f );
				hueSurface.setPixel( ivec2( x, y ), color );
			}
		}
		hueTex = gl::Texture2d::create( hueSurface );
	}
	
	// draw popup window
	SetNextWindowSize( ImVec2( 100, 80 ) );
	if( BeginPopupContextItem( "ColorPickerPopup" ) ){
		Image( svTex, ImVec2( texSize.x, texSize.y ) );
		
		ImGuiWindow* window = GetCurrentWindow();
		window->DrawList->AddCircle( GetItemBoxMin() + ImVec2( texSize.x * hsv.y, texSize.y * ( 1.0f - hsv.z ) ), 3, 0xffffffff );
		if( IsItemHovered() && IsMouseDown( 0 ) ){
			hsv.y			= (float) ( GetMousePos().x - GetItemRectMin().x ) / (float) GetItemRectSize().x;
			hsv.z			= 1.0f - (float) ( GetMousePos().y - GetItemRectMin().y ) / (float) GetItemRectSize().y;
			needTexUpdate	= true;
			changes			= true;
		}
		SameLine();
		Image( hueTex, ImVec2( 10, texSize.y ) );
		
		float hueY = hsv.x * (float) GetItemRectSize().y + GetItemBoxMin().y;
		window->DrawList->AddTriangleFilled( ImVec2( GetItemBoxMin().x - 3, hueY - 3 ), ImVec2( GetItemBoxMin().x - 3, hueY + 3 ), ImVec2( GetItemBoxMin().x + 1, hueY ), 0xffffffff );
		window->DrawList->AddTriangleFilled( ImVec2( GetItemBoxMax().x + 3, hueY - 3 ), ImVec2( GetItemBoxMax().x + 3, hueY + 3 ), ImVec2( GetItemBoxMax().x - 1, hueY ), 0xffffffff );
		if( IsItemHovered() && IsMouseDown( 0 ) ){
			hsv.x	= (float) ( GetMousePos().y - GetItemRectMin().y ) / (float) GetItemRectSize().y;
			changes = true;
		}
		
		hsv = glm::clamp( hsv, vec3(0), vec3(1) );
		
		ColorA converted = ColorA( ColorModel::CM_HSV, hsv.x, hsv.y, hsv.z );
		col[0] = converted.r;
		col[1] = converted.g;
		col[2] = converted.b;
		
		EndPopup();
	}
	
	return changes;
}

bool ImGui::TimelineFloat( const char* label, float* keyframes, int keyframes_count )
{
	ImVec2 graph_size = ImVec2(0,0);
	//float scale_min = FLT_MAX;
	//float scale_max = FLT_MAX;
	
	ImGuiState& g = *GImGui;
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;
	
	const ImGuiStyle& style = g.Style;
	
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	if (graph_size.x == 0.0f)
		graph_size.x = CalcItemWidth() + (style.FramePadding.x * 2);
	if (graph_size.y == 0.0f)
		graph_size.y = label_size.y + (style.FramePadding.y * 2);
	
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size.x, graph_size.y));
	const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, NULL))
		return false;
	
	RenderFrame(frame_bb.Min, frame_bb.Max, window->Color(ImGuiCol_FrameBg), true, style.FrameRounding);
	
	int res_w = ImMin((int)graph_size.x, keyframes_count);
	//if (plot_type == ImGuiPlotType_Lines)
		res_w -= 1;
	/*
	// Tooltip on hover
	int v_hovered = -1;
	if (IsMouseHoveringRect(inner_bb))
	{
		const float t = ImClamp((g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);
		const int v_idx = (int)(t * (values_count + ((plot_type == ImGuiPlotType_Lines) ? -1 : 0)));
		IM_ASSERT(v_idx >= 0 && v_idx < values_count);
		
		const float v0 = values_getter(data, (v_idx + values_offset) % values_count);
		const float v1 = values_getter(data, (v_idx + 1 + values_offset) % values_count);
		if (plot_type == ImGuiPlotType_Lines)
			SetTooltip("%d: %8.4g\n%d: %8.4g", v_idx, v0, v_idx+1, v1);
		else if (plot_type == ImGuiPlotType_Histogram)
			SetTooltip("%d: %8.4g", v_idx, v0);
		v_hovered = v_idx;
	}
	
	const float t_step = 1.0f / (float)res_w;
	
	float v0 = values_getter(data, (0 + values_offset) % values_count);
	float t0 = 0.0f;
	ImVec2 p0 = ImVec2( t0, 1.0f - ImSaturate((v0 - scale_min) / (scale_max - scale_min)) );
	
	const ImU32 col_base = window->Color((plot_type == ImGuiPlotType_Lines) ? ImGuiCol_PlotLines : ImGuiCol_PlotHistogram);
	const ImU32 col_hovered = window->Color((plot_type == ImGuiPlotType_Lines) ? ImGuiCol_PlotLinesHovered : ImGuiCol_PlotHistogramHovered);
	
	for (int n = 0; n < res_w; n++)
	{
		const float t1 = t0 + t_step;
		const int v_idx = (int)(t0 * values_count);
		IM_ASSERT(v_idx >= 0 && v_idx < values_count);
		const float v1 = values_getter(data, (v_idx + values_offset + 1) % values_count);
		const ImVec2 p1 = ImVec2( t1, 1.0f - ImSaturate((v1 - scale_min) / (scale_max - scale_min)) );
		
		// NB- Draw calls are merged together by the DrawList system.
		if (plot_type == ImGuiPlotType_Lines)
			window->DrawList->AddLine(ImLerp(inner_bb.Min, inner_bb.Max, p0), ImLerp(inner_bb.Min, inner_bb.Max, p1), v_hovered == v_idx ? col_hovered : col_base);
		else if (plot_type == ImGuiPlotType_Histogram)
			window->DrawList->AddRectFilled(ImLerp(inner_bb.Min, inner_bb.Max, p0), ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(p1.x, 1.0f))+ImVec2(-1,0), v_hovered == v_idx ? col_hovered : col_base);
		
		t0 = t1;
		p0 = p1;
	}
	
	// Text overlay
	if (overlay_text)
		RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, overlay_text, NULL, NULL, ImGuiAlign_Center);
	
	RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);*/
	
	return true;
}