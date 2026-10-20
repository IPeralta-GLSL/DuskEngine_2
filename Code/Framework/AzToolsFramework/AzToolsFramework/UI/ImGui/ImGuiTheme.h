/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <imgui/imgui.h>

// Single source of truth for ImGui panel colors. Values mirror
// Code/Editor/Style/EditorStylesheetVariables_Dark.json so the ImGui
// docks match the Qt editor chrome (accent green #4CAF50).
namespace AzToolsFramework
{
    namespace ImGuiTheme
    {
        inline const ImVec4 Accent = ImVec4(0.298f, 0.686f, 0.314f, 1.0f);        // #4CAF50
        inline const ImVec4 AccentHover = ImVec4(0.400f, 0.733f, 0.416f, 1.0f);   // #66BB6A
        inline const ImVec4 AccentMuted = ImVec4(0.298f, 0.686f, 0.314f, 0.5f);

        inline const ImVec4 WindowBg = ImVec4(0.106f, 0.106f, 0.118f, 1.0f);      // #1B1B1E
        inline const ImVec4 PanelBg = ImVec4(0.129f, 0.129f, 0.141f, 1.0f);       // #212124
        inline const ImVec4 InputBg = ImVec4(0.165f, 0.165f, 0.180f, 1.0f);       // #2A2A2E
        inline const ImVec4 HoverBg = ImVec4(0.149f, 0.149f, 0.169f, 1.0f);       // #26262B
        inline const ImVec4 HeaderBg = ImVec4(0.149f, 0.149f, 0.169f, 1.0f);      // #26262B
        inline const ImVec4 Border = ImVec4(0.208f, 0.208f, 0.231f, 1.0f);        // #35353B
        inline const ImVec4 Handle = ImVec4(0.227f, 0.227f, 0.255f, 1.0f);        // #3A3A41

        inline const ImVec4 TextNormal = ImVec4(0.831f, 0.831f, 0.847f, 1.0f);    // #D4D4D8
        inline const ImVec4 TextMuted = ImVec4(0.557f, 0.557f, 0.580f, 1.0f);     // #8E8E94
        inline const ImVec4 TextDisabled = ImVec4(0.353f, 0.353f, 0.376f, 1.0f);  // #5A5A60

        inline const ImVec4 Error = ImVec4(0.886f, 0.322f, 0.263f, 1.0f);         // #E25243
        inline const ImVec4 ErrorHover = ImVec4(0.933f, 0.396f, 0.337f, 1.0f);    // #ED6556
        inline const ImVec4 Warning = ImVec4(0.941f, 0.765f, 0.176f, 1.0f);       // #F0C32D
    } // namespace ImGuiTheme
} // namespace AzToolsFramework
