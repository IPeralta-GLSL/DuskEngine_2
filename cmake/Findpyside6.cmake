# Findpyside6.cmake - Provides PySide6 targets for Qt6 migration

set(pyside6_FOUND TRUE)
set(PYSIDE6_AVAILABLE TRUE)

if(NOT TARGET 3rdParty::pyside6)
    add_library(3rdParty::pyside6 INTERFACE IMPORTED)
endif()
if(NOT TARGET 3rdParty::pyside6::Tools)
    add_library(3rdParty::pyside6::Tools INTERFACE IMPORTED)
endif()
