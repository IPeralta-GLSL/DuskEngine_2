#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME ComponentEntityEditorPlugin MODULE
    NAMESPACE Legacy
    OUTPUT_SUBDIRECTORY EditorPlugins
    AUTOMOC
    FILES_CMAKE
        componententityeditorplugin_files.cmake
    COMPILE_DEFINITIONS
        PRIVATE
            PLUGIN_EXPORTS
    INCLUDE_DIRECTORIES
        PUBLIC
            .
        PRIVATE
            ${LY_ROOT_FOLDER}/SDK/imgui-node-editor
            ${LY_ROOT_FOLDER}/plugins/ImGui/External/ImGui/v1.82/imgui
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Widgets
            3rdParty::Qt::OpenGL
            AZ::AzCore
            AZ::AzToolsFramework
            Legacy::CryCommon
            Legacy::EditorLib
            AZ::AtomCore
            Gem::Atom_RPI.Public
            Gem::AtomToolsFramework.Static
            Gem::LmbrCentral.Editor
            imgui_node_editor_static
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral.Editor
)

ly_add_dependencies(Editor ComponentEntityEditorPlugin)
set_property(GLOBAL APPEND PROPERTY LY_EDITOR_PLUGINS $<TARGET_FILE_NAME:Legacy::ComponentEntityEditorPlugin>)