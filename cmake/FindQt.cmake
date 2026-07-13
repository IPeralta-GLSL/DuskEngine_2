# FindQt.cmake - Wrapper for Qt6 when O3DE_QT_VERSION=6
# Provides find_package(Qt) compatibility and Qt5-era helper functions

if(O3DE_QT_VERSION STREQUAL "6")
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg Xml Concurrent Network OpenGL PrintSupport)

    set(Qt_FOUND TRUE)
    set(Qt_VERSION ${Qt6_VERSION})
    set(QT_VERSION_MAJOR 6)

    foreach(_module Core Gui Widgets Svg Xml Concurrent Network OpenGL PrintSupport)
        if(NOT TARGET Qt::${_module})
            add_library(Qt::${_module} INTERFACE IMPORTED)
            target_link_libraries(Qt::${_module} INTERFACE Qt6::${_module})
        endif()
        if(NOT TARGET 3rdParty::Qt::${_module})
            add_library(3rdParty::Qt::${_module} INTERFACE IMPORTED)
            target_link_libraries(3rdParty::Qt::${_module} INTERFACE Qt6::${_module})
        endif()
    endforeach()

    find_package(Qt6 COMPONENTS OpenGLWidgets QUIET)
    find_package(Qt6 COMPONENTS Test QUIET)
    if(NOT Qt6_Test_FOUND)
        # Create empty stub if Qt6Test not found
        if(NOT TARGET 3rdParty::Qt::Test)
            add_library(3rdParty::Qt::Test INTERFACE IMPORTED)
        endif()
    endif()

    foreach(_optional_module OpenGLWidgets Test Concurrent Network OpenGL PrintSupport)
        if(Qt6_${_optional_module}_FOUND)
            if(NOT TARGET Qt::${_optional_module})
                add_library(Qt::${_optional_module} INTERFACE IMPORTED)
                target_link_libraries(Qt::${_optional_module} INTERFACE Qt6::${_optional_module})
            endif()
            if(NOT TARGET 3rdParty::Qt::${_optional_module})
                add_library(3rdParty::Qt::${_optional_module} INTERFACE IMPORTED)
                target_link_libraries(3rdParty::Qt::${_optional_module} INTERFACE Qt6::${_optional_module})
            endif()
        endif()
    endforeach()

    find_package(Qt6 COMPONENTS LinguistTools QUIET)
    if(Qt6_LinguistTools_FOUND)
        set(QT_UIC_EXECUTABLE Qt6::uic)
        set(QT_MOC_EXECUTABLE Qt6::moc)
        set(QT_RCC_EXECUTABLE Qt6::rcc)
        set(QT_LRELEASE_EXECUTABLE Qt6::lrelease)
        set(QT_LUPDATE_EXECUTABLE Qt6::lupdate)
    else()
        find_program(QT_UIC_EXECUTABLE uic)
        find_program(QT_MOC_EXECUTABLE moc)
    endif()

    function(ly_qt_uic_target TARGET)
        get_target_property(all_ui_sources ${TARGET} SOURCES)
        list(FILTER all_ui_sources INCLUDE REGEX "^.*\\.ui$")
        if(NOT all_ui_sources)
            message(FATAL_ERROR "Target ${TARGET} contains AUTOUIC but doesnt have any .ui file")
        endif()

        if(AUTOGEN_BUILD_DIR)
            set(gen_dir ${AUTOGEN_BUILD_DIR})
        else()
            set(gen_dir ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_autogen/include)
        endif()

        foreach(ui_source ${all_ui_sources})
            get_filename_component(filename ${ui_source} NAME_WE)
            get_filename_component(dir ${ui_source} DIRECTORY)
            if(IS_ABSOLUTE ${dir})
                file(RELATIVE_PATH dir ${CMAKE_CURRENT_SOURCE_DIR} ${dir})
            endif()
            set(outfolder ${gen_dir}/${dir})
            set(outfile ${outfolder}/ui_${filename}.h)
            get_filename_component(infile ${ui_source} ABSOLUTE)
            file(MAKE_DIRECTORY ${outfolder})
            add_custom_command(OUTPUT ${outfile}
                COMMAND ${QT_UIC_EXECUTABLE} -o ${outfile} ${infile}
                MAIN_DEPENDENCY ${infile} VERBATIM
                COMMENT "UIC ${infile}"
            )
            set_source_files_properties(${infile} PROPERTIES SKIP_AUTOUIC TRUE)
            set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOMOC TRUE SKIP_AUTOUIC TRUE GENERATED TRUE)
            list(APPEND all_ui_wrapped_sources ${outfile})
        endforeach()

        target_sources(${TARGET} PRIVATE ${all_ui_wrapped_sources})
        source_group("Generated Files" FILES ${all_ui_wrapped_sources})

        get_property(has_includes TARGET ${TARGET} PROPERTY INCLUDE_DIRECTORIES SET)
        if(has_includes)
            get_property(all_include_directories TARGET ${TARGET} PROPERTY INCLUDE_DIRECTORIES)
            foreach(dir ${all_include_directories})
                if(IS_ABSOLUTE ${dir})
                    file(RELATIVE_PATH dir ${CMAKE_CURRENT_SOURCE_DIR} ${dir})
                endif()
                list(APPEND new_includes ${gen_dir}/${dir})
            endforeach()
        endif()
        list(APPEND new_includes ${gen_dir})
        target_include_directories(${TARGET} PRIVATE ${new_includes})
    endfunction()

    function(ly_add_translations)
        # Stub - translation support not needed for Qt6 migration testing
    endfunction()

    return()
endif()

message(FATAL_ERROR "FindQt.cmake: Qt5 package not found and O3DE_QT_VERSION is not 6")
