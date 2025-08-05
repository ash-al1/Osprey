#pragma once

#include "imgui/imgui.h"

void SetDraculaTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Dracula color palette
    const ImVec4 background     = ImVec4(0.156f, 0.165f, 0.212f, 1.00f); // #282a36
    const ImVec4 currentLine    = ImVec4(0.267f, 0.278f, 0.353f, 1.00f); // #44475a
    const ImVec4 selection      = ImVec4(0.267f, 0.278f, 0.353f, 1.00f); // #44475a
    const ImVec4 foreground     = ImVec4(0.973f, 0.973f, 0.949f, 1.00f); // #f8f8f2
    const ImVec4 comment        = ImVec4(0.384f, 0.447f, 0.643f, 1.00f); // #6272a4
    const ImVec4 cyan           = ImVec4(0.545f, 0.914f, 0.992f, 1.00f); // #8be9fd
    const ImVec4 green          = ImVec4(0.314f, 0.980f, 0.482f, 1.00f); // #50fa7b
    const ImVec4 orange         = ImVec4(1.000f, 0.722f, 0.424f, 1.00f); // #ffb86c
    const ImVec4 pink           = ImVec4(1.000f, 0.475f, 0.776f, 1.00f); // #ff79c6
    const ImVec4 purple         = ImVec4(0.741f, 0.576f, 0.976f, 1.00f); // #bd93f9
    const ImVec4 red            = ImVec4(1.000f, 0.333f, 0.333f, 1.00f); // #ff5555
    const ImVec4 yellow         = ImVec4(0.945f, 0.980f, 0.549f, 1.00f); // #f1fa8c

    // Main colors
    colors[ImGuiCol_Text]                   = foreground;
    colors[ImGuiCol_TextDisabled]           = comment;
    colors[ImGuiCol_WindowBg]               = background;
    colors[ImGuiCol_ChildBg]                = background;
    colors[ImGuiCol_PopupBg]                = background;
    colors[ImGuiCol_Border]                 = currentLine;
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = currentLine;
    colors[ImGuiCol_FrameBgHovered]         = selection;
    colors[ImGuiCol_FrameBgActive]          = selection;
    colors[ImGuiCol_TitleBg]                = background;
    colors[ImGuiCol_TitleBgActive]          = currentLine;
    colors[ImGuiCol_TitleBgCollapsed]       = background;
    colors[ImGuiCol_MenuBarBg]              = background;
    colors[ImGuiCol_ScrollbarBg]            = background;
    colors[ImGuiCol_ScrollbarGrab]          = currentLine;
    colors[ImGuiCol_ScrollbarGrabHovered]   = selection;
    colors[ImGuiCol_ScrollbarGrabActive]    = comment;
    colors[ImGuiCol_CheckMark]              = green;
    colors[ImGuiCol_SliderGrab]             = purple;
    colors[ImGuiCol_SliderGrabActive]       = pink;
    colors[ImGuiCol_Button]                 = currentLine;
    colors[ImGuiCol_ButtonHovered]          = selection;
    colors[ImGuiCol_ButtonActive]           = comment;
    colors[ImGuiCol_Header]                 = currentLine;
    colors[ImGuiCol_HeaderHovered]          = selection;
    colors[ImGuiCol_HeaderActive]           = comment;
    colors[ImGuiCol_Separator]              = currentLine;
    colors[ImGuiCol_SeparatorHovered]       = cyan;
    colors[ImGuiCol_SeparatorActive]        = cyan;
    colors[ImGuiCol_ResizeGrip]             = currentLine;
    colors[ImGuiCol_ResizeGripHovered]      = selection;
    colors[ImGuiCol_ResizeGripActive]       = comment;
    colors[ImGuiCol_Tab]                    = background;
    colors[ImGuiCol_TabHovered]             = selection;
    colors[ImGuiCol_TabActive]              = currentLine;
    colors[ImGuiCol_TabUnfocused]           = background;
    colors[ImGuiCol_TabUnfocusedActive]     = currentLine;
    colors[ImGuiCol_PlotLines]              = cyan;
    colors[ImGuiCol_PlotLinesHovered]       = yellow;
    colors[ImGuiCol_PlotHistogram]          = cyan;
    colors[ImGuiCol_PlotHistogramHovered]   = yellow;
    colors[ImGuiCol_TableHeaderBg]          = currentLine;
    colors[ImGuiCol_TableBorderStrong]      = currentLine;
    colors[ImGuiCol_TableBorderLight]       = selection;
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]         = selection;
    colors[ImGuiCol_DragDropTarget]         = yellow;
    colors[ImGuiCol_NavHighlight]           = cyan;
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    // Style adjustments
    style.WindowRounding    = 5.0f;
    style.FrameRounding     = 3.0f;
    style.GrabRounding      = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.TabRounding       = 3.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.PopupBorderSize   = 1.0f;
}
