# Qt6 wrapper for O3DE's find_package(Qt) system
# When O3DE_QT_VERSION=6, this wraps Qt6 modules as Qt::* targets

if(NOT O3DE_QT_VERSION STREQUAL "6")
    message(FATAL_ERROR "QtConfig.cmake wrapper should only be used with O3DE_QT_VERSION=6")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg Xml)

# Create Qt::* alias targets that point to Qt6::*
foreach(_module Core Gui Widgets Svg Xml)
    if(NOT TARGET Qt::${_module})
        add_library(Qt::${_module} ALIAS Qt6::${_module})
    endif()
endforeach()

# Handle optional modules
find_package(Qt6 COMPONENTS OpenGLWidgets QUIET)
if(Qt6_OpenGLWidgets_FOUND AND NOT TARGET Qt::OpenGLWidgets)
    add_library(Qt::OpenGLWidgets ALIAS Qt6::OpenGLWidgets)
endif()

find_package(Qt6 COMPONENTS LinguistTools QUIET)

set(Qt_FOUND TRUE)
set(Qt_VERSION ${Qt6_VERSION})
