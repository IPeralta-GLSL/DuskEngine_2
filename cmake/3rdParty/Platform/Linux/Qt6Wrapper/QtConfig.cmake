# Qt6 wrapper for O3DE's find_package(Qt) system
# When O3DE_QT_VERSION=6, this wraps Qt6 modules as Qt::* targets

if(NOT O3DE_QT_VERSION STREQUAL "6")
    message(FATAL_ERROR "QtConfig.cmake wrapper should only be used with O3DE_QT_VERSION=6")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg SvgWidgets Xml)

# Suppress Qt deprecation warnings to avoid -Werror failures
add_compile_definitions(QT_NO_DEPRECATED_WARNINGS)

# Ensure Qt6 tools (uic, moc, rcc) are used instead of system Qt5 tools
find_package(Qt6 COMPONENTS Tools REQUIRED)
set(QT_UIC_EXECUTABLE /usr/lib/qt6/uic CACHE FILEPATH "Qt6 uic executable" FORCE)
set(QT_MOC_EXECUTABLE /usr/lib/qt6/moc CACHE FILEPATH "Qt6 moc executable" FORCE)
set(QT_RCC_EXECUTABLE /usr/lib/qt6/rcc CACHE FILEPATH "Qt6 rcc executable" FORCE)

# Create Qt::* alias targets that point to Qt6::*
foreach(_module Core Gui Widgets Svg SvgWidgets Xml)
    if(NOT TARGET Qt::${_module})
        add_library(Qt::${_module} ALIAS Qt6::${_module})
    endif()
endforeach()

# Handle optional modules
find_package(Qt6 COMPONENTS OpenGLWidgets QUIET)
if(Qt6_OpenGLWidgets_FOUND AND NOT TARGET Qt::OpenGLWidgets)
    add_library(Qt::OpenGLWidgets ALIAS Qt6::OpenGLWidgets)
endif()

find_package(Qt6 COMPONENTS SvgWidgets QUIET)
if(Qt6_SvgWidgets_FOUND AND NOT TARGET Qt::SvgWidgets)
    add_library(Qt::SvgWidgets ALIAS Qt6::SvgWidgets)
endif()

find_package(Qt6 COMPONENTS LinguistTools QUIET)

set(Qt_FOUND TRUE)
set(Qt_VERSION ${Qt6_VERSION})
