#!/bin/bash
set -e

SDK_DIR="$(cd "$(dirname "$0")/../../../FidelityFX-SDK/sdk" && pwd)"
SC_EXE="${SDK_DIR}/tools/binary_store/FidelityFX_SC.exe"
GPU_PATH="${SDK_DIR}/include/FidelityFX/gpu"
OUTPUT_DIR="${SDK_DIR}/build_shaders_vk"

mkdir -p "${OUTPUT_DIR}"

FSR3UP_BASE_ARGS=(
    -reflection -deps=gcc -DFFX_GPU=1
    -DFFX_FSR3UPSCALER_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF=0
    -DFFX_FSR3UPSCALER_OPTION_ACCUMULATE_SAMPLERS_USE_DATA_HALF=0
    -DFFX_FSR3UPSCALER_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF=1
    -DFFX_FSR3UPSCALER_OPTION_POSTPROCESSLOCKSTATUS_SAMPLERS_USE_DATA_HALF=0
    -DFFX_FSR3UPSCALER_OPTION_UPSAMPLE_USE_LANCZOS_TYPE=2
)

FSR3UP_API_ARGS=(
    -compiler=glslang -e CS --target-env vulkan1.2 -S comp -Os -DFFX_GLSL=1
)

FSR3UP_PERM_ARGS=(
    '-DFFX_FSR3UPSCALER_OPTION_REPROJECT_USE_LANCZOS_TYPE={0,1}'
    '-DFFX_FSR3UPSCALER_OPTION_HDR_COLOR_INPUT={0,1}'
    '-DFFX_FSR3UPSCALER_OPTION_LOW_RESOLUTION_MOTION_VECTORS={0,1}'
    '-DFFX_FSR3UPSCALER_OPTION_JITTERED_MOTION_VECTORS={0,1}'
    '-DFFX_FSR3UPSCALER_OPTION_INVERTED_DEPTH={0,1}'
    '-DFFX_FSR3UPSCALER_OPTION_APPLY_SHARPENING={0,1}'
)

FSR3UP_INCLUDES=(
    "-I${GPU_PATH}"
    "-I${GPU_PATH}/fsr3upscaler"
)

FSR3UP_SHADERS=(
    ffx_fsr3upscaler_accumulate_pass
    ffx_fsr3upscaler_autogen_reactive_pass
    ffx_fsr3upscaler_debug_view_pass
    ffx_fsr3upscaler_luma_instability_pass
    ffx_fsr3upscaler_luma_pyramid_pass
    ffx_fsr3upscaler_prepare_inputs_pass
    ffx_fsr3upscaler_prepare_reactivity_pass
    ffx_fsr3upscaler_rcas_pass
    ffx_fsr3upscaler_shading_change_pass
    ffx_fsr3upscaler_shading_change_pyramid_pass
)

compile_shader() {
    local shader_file="$1"
    local name_suffix="$2"
    local ffx_half="$3"
    shift 3
    local base_args=("$@")

    local shader_name
    shader_name=$(basename "${shader_file}" .glsl)
    local full_name="${shader_name}${name_suffix}"

    echo "  Compiling ${full_name} (FFX_HALF=${ffx_half})..."
    wine "${SC_EXE}" \
        "${base_args[@]}" \
        "-name=${full_name}" \
        "-DFFX_HALF=${ffx_half}" \
        "-output=${OUTPUT_DIR}" \
        "${shader_file}" 2>&1 | grep -v "fixme:" || true
}

echo "=== Compiling FSR3 Upscaler shaders ==="
for shader in "${FSR3UP_SHADERS[@]}"; do
    shader_file="${SDK_DIR}/src/backends/vk/shaders/fsr3upscaler/${shader}.glsl"
    echo "Processing ${shader}..."

    compile_shader "${shader_file}" "" 0 \
        "${FSR3UP_BASE_ARGS[@]}" "${FSR3UP_API_ARGS[@]}" "${FSR3UP_PERM_ARGS[@]}" "${FSR3UP_INCLUDES[@]}"

    compile_shader "${shader_file}" "_wave64" 0 \
        "${FSR3UP_BASE_ARGS[@]}" "${FSR3UP_API_ARGS[@]}" "${FSR3UP_PERM_ARGS[@]}" "${FSR3UP_INCLUDES[@]}"

    compile_shader "${shader_file}" "_16bit" 1 \
        "${FSR3UP_BASE_ARGS[@]}" "${FSR3UP_API_ARGS[@]}" "${FSR3UP_PERM_ARGS[@]}" "${FSR3UP_INCLUDES[@]}"

    compile_shader "${shader_file}" "_wave64_16bit" 1 \
        "${FSR3UP_BASE_ARGS[@]}" "${FSR3UP_API_ARGS[@]}" "${FSR3UP_PERM_ARGS[@]}" "${FSR3UP_INCLUDES[@]}"
done

TOTAL_FSR3=$(find "${OUTPUT_DIR}" -name "ffx_fsr3upscaler_*_permutations.h" | wc -l)
echo "=== Done! Generated ${TOTAL_FSR3} FSR3 Upscaler permutation headers ==="
