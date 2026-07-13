#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Qt 5/6 Dual Compatibility Layer

set(O3DE_QT_VERSION "5" CACHE STRING "Qt version to use (5 or 6)")
set_property(CACHE O3DE_QT_VERSION PROPERTY STRINGS 5 6)

if(O3DE_QT_VERSION STREQUAL "6")
    message(STATUS "O3DE: Using system Qt 6")
    
    # Add cmake/ dir to module path so FindQt.cmake wrapper is found
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../")
    
    add_compile_definitions(O3DE_QT6)
    
else()
    message(STATUS "O3DE: Using pre-built Qt 5 package")
endif()
