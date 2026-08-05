#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Qt 6 Compatibility Layer

set(O3DE_QT_VERSION "6" CACHE STRING "Qt version to use (6 only)")
set_property(CACHE O3DE_QT_VERSION PROPERTY STRINGS 6)

message(STATUS "O3DE: Using system Qt 6")

# Add cmake/ dir to module path so FindQt.cmake wrapper is found
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../")

# Create PySide6 targets for system Qt6
if(NOT TARGET 3rdParty::pyside6)
    add_library(3rdParty::pyside6 INTERFACE IMPORTED)
endif()

if(NOT TARGET 3rdParty::pyside6::Tools)
    add_library(3rdParty::pyside6::Tools INTERFACE IMPORTED)
    find_program(PYSIDE6_LUPDATE_EXECUTABLE lupdate-qt6)
    find_program(PYSIDE6_LRELEASE_EXECUTABLE lrelease-qt6)
    if(PYSIDE6_LUPDATE_EXECUTABLE)
        set_target_properties(3rdParty::pyside6::Tools PROPERTIES
            LUPDATE_EXECUTABLE ${PYSIDE6_LUPDATE_EXECUTABLE}
            LRELEASE_EXECUTABLE ${PYSIDE6_LRELEASE_EXECUTABLE})
    endif()
endif()

add_compile_definitions(O3DE_QT6)
