set(XESS_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../Xess-SDK")

if(NOT EXISTS "${XESS_SDK_ROOT}/inc/xess/xess.h")
    message(FATAL_ERROR "Intel XeSS SDK not found at ${XESS_SDK_ROOT}. "
        "Please place the XeSS SDK in the engine root as 'Xess-SDK/'")
    set(XeSSSDK_FOUND FALSE)
    return()
endif()

find_package(Vulkan REQUIRED)

# XeSS is a prebuilt library — we only need headers for compilation.
# The shared library (libxess.so / libxess.dll) is loaded at runtime via dlopen/LoadLibrary.
add_library(XeSSSDK_internal INTERFACE)

target_include_directories(XeSSSDK_internal
    INTERFACE
        ${XESS_SDK_ROOT}/inc
)

target_link_libraries(XeSSSDK_internal
    INTERFACE
        Vulkan::Vulkan
)

add_library(3rdParty::XeSSSDK ALIAS XeSSSDK_internal)
set(XeSSSDK_FOUND TRUE)
