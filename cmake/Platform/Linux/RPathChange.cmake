#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT ${CMAKE_ARGC} EQUAL 6)
    message(FATAL_ERROR "RPathChange script called with the wrong number of arguments")
endif()

# Only patch binaries that actually have an RPATH/RUNPATH entry; system binaries
# (e.g. /usr/bin/lrelease) have none and do not need patching.
execute_process(
    COMMAND readelf -d "${CMAKE_ARGV3}"
    OUTPUT_VARIABLE _rpath_change_elf_out
    ERROR_QUIET
    RESULT_VARIABLE _rpath_change_elf_result
)
if(_rpath_change_elf_result EQUAL 0 AND _rpath_change_elf_out MATCHES "\\(RPATH\\)|\\(RUNPATH\\)")
    file(RPATH_CHANGE FILE "${CMAKE_ARGV3}" OLD_RPATH "${CMAKE_ARGV4}" NEW_RPATH "${CMAKE_ARGV5}")
endif()