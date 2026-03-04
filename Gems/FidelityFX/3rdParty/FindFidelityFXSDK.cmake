set(FFX_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../FidelityFX-SDK/sdk")

if(NOT EXISTS "${FFX_SDK_ROOT}/include/FidelityFX/host/ffx_fsr3upscaler.h")
    message(FATAL_ERROR "FidelityFX SDK not found at ${FFX_SDK_ROOT}")
    set(FidelityFXSDK_FOUND FALSE)
    return()
endif()

find_package(Vulkan REQUIRED)

set(FFX_INCLUDE_PATH ${FFX_SDK_ROOT}/include)
set(FFX_SHARED_PATH ${FFX_SDK_ROOT}/src/shared)
set(FFX_HOST_PATH ${FFX_SDK_ROOT}/include/FidelityFX/host)
set(FFX_GPU_PATH ${FFX_SDK_ROOT}/include/FidelityFX/gpu)
set(FFX_COMPONENTS_PATH ${FFX_SDK_ROOT}/src/components)
set(FFX_SRC_BACKENDS_PATH ${FFX_SDK_ROOT}/src/backends)

set(FFX_SHARED_SOURCES
    ${FFX_SHARED_PATH}/ffx_assert.cpp
    ${FFX_SHARED_PATH}/ffx_breadcrumbs_list.cpp
    ${FFX_SHARED_PATH}/ffx_breadcrumbs_list.h
    ${FFX_SHARED_PATH}/ffx_message.cpp
    ${FFX_SHARED_PATH}/ffx_object_management.cpp
    ${FFX_SHARED_PATH}/ffx_object_management.h
)

set(FFX_FSR3UPSCALER_SOURCES
    ${FFX_COMPONENTS_PATH}/fsr3upscaler/ffx_fsr3upscaler.cpp
    ${FFX_COMPONENTS_PATH}/fsr3upscaler/ffx_fsr3upscaler_private.h
)

set(FFX_SPD_SOURCES
    ${FFX_COMPONENTS_PATH}/spd/ffx_spd.cpp
)

set(FFX_BACKEND_SHARED_SOURCES
    ${FFX_SRC_BACKENDS_PATH}/shared/ffx_shader_blobs.cpp
    ${FFX_SRC_BACKENDS_PATH}/shared/ffx_shader_blobs.h
    ${FFX_SRC_BACKENDS_PATH}/shared/blob_accessors/ffx_fsr3upscaler_shaderblobs.cpp
    ${FFX_SRC_BACKENDS_PATH}/shared/blob_accessors/ffx_fsr3upscaler_shaderblobs.h
    ${FFX_SRC_BACKENDS_PATH}/shared/blob_accessors/ffx_spd_shaderblobs.cpp
    ${FFX_SRC_BACKENDS_PATH}/shared/blob_accessors/ffx_spd_shaderblobs.h
)

set(FFX_VK_BACKEND_SOURCES
    ${FFX_SRC_BACKENDS_PATH}/vk/ffx_vk.cpp
)

set(FFX_STUB_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/ffx_framegen_stub.cpp
)

add_library(FidelityFXSDK_internal STATIC
    ${FFX_SHARED_SOURCES}
    ${FFX_FSR3UPSCALER_SOURCES}
    ${FFX_SPD_SOURCES}
    ${FFX_BACKEND_SHARED_SOURCES}
    ${FFX_VK_BACKEND_SOURCES}
    ${FFX_STUB_SOURCES}
)

set(FFX_SHADER_OUTPUT_PATH ${FFX_SDK_ROOT}/build_shaders_vk)

target_include_directories(FidelityFXSDK_internal
    PUBLIC
        ${FFX_INCLUDE_PATH}
        ${FFX_SHARED_PATH}
    PRIVATE
        ${FFX_SRC_BACKENDS_PATH}/shared
        ${FFX_SRC_BACKENDS_PATH}/shared/blob_accessors
        ${FFX_COMPONENTS_PATH}
        ${FFX_COMPONENTS_PATH}/fsr3upscaler
        ${FFX_COMPONENTS_PATH}/spd
        ${FFX_SHADER_OUTPUT_PATH}
)

target_link_libraries(FidelityFXSDK_internal
    PUBLIC
        Vulkan::Vulkan
)

target_compile_definitions(FidelityFXSDK_internal
    PRIVATE
        FFX_FSR3UPSCALER=1
        FFX_SPD=1
        FFX_GCC
)

target_compile_options(FidelityFXSDK_internal
    PRIVATE
        -Wno-unknown-pragmas
        -Wno-missing-braces
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-deprecated-declarations
        -Wno-absolute-value
        -Wno-error
        -include ${CMAKE_CURRENT_LIST_DIR}/ffx_platform_linux.h
        -std=c++17
)

set_target_properties(FidelityFXSDK_internal PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_library(3rdParty::FidelityFXSDK ALIAS FidelityFXSDK_internal)
set(FidelityFXSDK_FOUND TRUE)
