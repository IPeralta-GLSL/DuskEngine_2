# Findpyside2.cmake - Stub for when PySide2 is not available (Qt6 migration)

set(pyside2_FOUND FALSE)
set(PYSIDE2_AVAILABLE FALSE)

if(NOT TARGET 3rdParty::pyside2)
    add_library(3rdParty::pyside2 INTERFACE IMPORTED)
endif()
if(NOT TARGET 3rdParty::pyside2::Tools)
    add_library(3rdParty::pyside2::Tools INTERFACE IMPORTED)
endif()
