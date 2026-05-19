#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    dllmain.cpp
    ComponentEntityEditorPlugin.h
    ComponentEntityEditorPlugin.cpp
    ContextMenuHandlers.h
    ContextMenuHandlers.cpp
    SandboxIntegration.h
    SandboxIntegration.cpp
    UI/QComponentEntityEditorMainWindow.h
    UI/QComponentEntityEditorMainWindow.cpp
    UI/QComponentEntityEditorOutlinerWindow.h
    UI/QComponentEntityEditorOutlinerWindow.cpp
    UI/AssetCatalogModel.h
    UI/AssetCatalogModel.cpp
    UI/ComponentPalette/ComponentPaletteSettings.h
    ${LY_ROOT_FOLDER}/Code/Framework/AzToolsFramework/AzToolsFramework/UI/ImGui/ImGuiQtOverlay.h
    ${LY_ROOT_FOLDER}/Code/Framework/AzToolsFramework/AzToolsFramework/UI/ImGui/ImGuiQtOverlay.cpp
    ${LY_ROOT_FOLDER}/Code/Framework/AzToolsFramework/AzToolsFramework/UI/ImGui/ImGuiEntityOutliner.h
    ${LY_ROOT_FOLDER}/Code/Framework/AzToolsFramework/AzToolsFramework/UI/ImGui/ImGuiEntityOutliner.cpp
    ${LY_ROOT_FOLDER}/Code/Framework/AzToolsFramework/AzToolsFramework/UI/ImGui/ImGuiEntityInspector.h
    ${LY_ROOT_FOLDER}/Code/Framework/AzToolsFramework/AzToolsFramework/UI/ImGui/ImGuiEntityInspector.cpp
    ${LY_ROOT_FOLDER}/Code/Framework/AzToolsFramework/AzToolsFramework/UI/ImGui/ImGuiEditorDockWidget.h
    ${LY_ROOT_FOLDER}/Code/Framework/AzToolsFramework/AzToolsFramework/UI/ImGui/ImGuiEditorDockWidget.cpp
    ${LY_ROOT_FOLDER}/Code/Framework/AzToolsFramework/AzToolsFramework/UI/ImGui/imgui_impl_opengl3.h
    ${LY_ROOT_FOLDER}/Code/Framework/AzToolsFramework/AzToolsFramework/UI/ImGui/imgui_impl_opengl3.cpp
)
