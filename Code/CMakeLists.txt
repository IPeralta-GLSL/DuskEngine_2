#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

add_subdirectory(Framework)
add_subdirectory(LauncherUnified)
add_subdirectory(Legacy)
add_subdirectory(${LY_ROOT_FOLDER}/SDK/imgui-node-editor ${CMAKE_BINARY_DIR}/SDK/imgui-node-editor)
add_subdirectory(Editor)
add_subdirectory(Tools)
