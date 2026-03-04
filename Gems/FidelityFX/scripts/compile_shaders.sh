#!/bin/bash
set -e

SDK_DIR="$(cd "$(dirname "$0")/../../../FidelityFX-SDK/sdk" && pwd)"
SC_EXE="${SDK_DIR}/tools/binary_store/FidelityFX_SC.exe"
GPU_PATH="${SDK_DIR}/include/FidelityFX/gpu"
OUTPUT_DIR="${SDK_DIR}/build_shaders_vk"

mkdir -p "${OUTPUT_DIR}"

FSR2_BASE_ARGS=(
    -reflection -deps=gcc -DFFX_GPU=1
    -DFFX_FSR2_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF=0
    -DFFX_FSR2_OPTION_ACCUMULATE_SAMPLERS_USE_DATA_HALF=0
    -DFFX_FSR2_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF=1
    -DFFX_FSR2_OPTION_POSTPROCESSLOCKSTATUS_SAMPLERS_USE_DATA_HALF=0
    -DFFX_FSR2_OPTION_UPSAMPLE_USE_LANCZOS_TYPE=2
)

FSR2_API_ARGS=(
    -compiler=glslang -e CS --target-env vulkan1.2 -S comp -Os -DFFX_GLSL=1
)

FSR2_PERM_ARGS=(
    '-DFFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE={0,1}'
    '-DFFX_FSR2_OPTION_HDR_COLOR_INPUT={0,1}'
    '-DFFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS={0,1}'
    '-DFFX_FSR2_OPTION_JITTERED_MOTION_VECTORS={0,1}'
    '-DFFX_FSR2_OPTION_INVERTED_DEPTH={0,1}'
    '-DFFX_FSR2_OPTION_APPLY_SHARPENING={0,1}'
)

FSR2_INCLUDES=(
    "-I${GPU_PATH}"
    "-I${GPU_PATH}/fsr2"
)

SPD_BASE_ARGS=(
    -reflection -deps=gcc -DFFX_GPU=1
)

SPD_API_ARGS=(
    -compiler=glslang -e CS --target-env vulkan1.2 -S comp -Os -DFFX_GLSL=1
)

SPD_PERM_ARGS=(
    '-DFFX_SPD_OPTION_LINEAR_SAMPLE={0,1}'
    '-DFFX_SPD_OPTION_WAVE_INTEROP_LDS={0,1}'
    '-DFFX_SPD_OPTION_DOWNSAMPLE_FILTER={0,1,2}'
)

SPD_INCLUDES=(
    "-I${GPU_PATH}"
    "-I${GPU_PATH}/spd"
)

FSR2_SHADERS=(
    ffx_fsr2_accumulate_pass
    ffx_fsr2_autogen_reactive_pass
    ffx_fsr2_compute_luminance_pyramid_pass
    ffx_fsr2_depth_clip_pass
    ffx_fsr2_lock_pass
    ffx_fsr2_rcas_pass
    ffx_fsr2_reconstruct_previous_depth_pass
    ffx_fsr2_tcr_autogen_pass
)

SPD_SHADERS=(
    ffx_spd_downsample_pass
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

echo "=== Compiling FSR2 shaders ==="
for shader in "${FSR2_SHADERS[@]}"; do
    shader_file="${SDK_DIR}/src/backends/vk/shaders/fsr2/${shader}.glsl"
    echo "Processing ${shader}..."

    compile_shader "${shader_file}" "" 0 \
        "${FSR2_BASE_ARGS[@]}" "${FSR2_API_ARGS[@]}" "${FSR2_PERM_ARGS[@]}" "${FSR2_INCLUDES[@]}"

    compile_shader "${shader_file}" "_wave64" 0 \
        "${FSR2_BASE_ARGS[@]}" "${FSR2_API_ARGS[@]}" "${FSR2_PERM_ARGS[@]}" "${FSR2_INCLUDES[@]}"

    compile_shader "${shader_file}" "_16bit" 1 \
        "${FSR2_BASE_ARGS[@]}" "${FSR2_API_ARGS[@]}" "${FSR2_PERM_ARGS[@]}" "${FSR2_INCLUDES[@]}"

    compile_shader "${shader_file}" "_wave64_16bit" 1 \
        "${FSR2_BASE_ARGS[@]}" "${FSR2_API_ARGS[@]}" "${FSR2_PERM_ARGS[@]}" "${FSR2_INCLUDES[@]}"
done

echo "=== Compiling SPD shaders ==="
for shader in "${SPD_SHADERS[@]}"; do
    shader_file="${SDK_DIR}/src/backends/vk/shaders/spd/${shader}.glsl"
    echo "Processing ${shader}..."

    compile_shader "${shader_file}" "" 0 \
        "${SPD_BASE_ARGS[@]}" "${SPD_API_ARGS[@]}" "${SPD_PERM_ARGS[@]}" "${SPD_INCLUDES[@]}"

    compile_shader "${shader_file}" "_wave64" 0 \
        "${SPD_BASE_ARGS[@]}" "${SPD_API_ARGS[@]}" "${SPD_PERM_ARGS[@]}" "${SPD_INCLUDES[@]}"

    compile_shader "${shader_file}" "_16bit" 1 \
        "${SPD_BASE_ARGS[@]}" "${SPD_API_ARGS[@]}" "${SPD_PERM_ARGS[@]}" "${SPD_INCLUDES[@]}"

    compile_shader "${shader_file}" "_wave64_16bit" 1 \
        "${SPD_BASE_ARGS[@]}" "${SPD_API_ARGS[@]}" "${SPD_PERM_ARGS[@]}" "${SPD_INCLUDES[@]}"
done

TOTAL_HEADERS=$(find "${OUTPUT_DIR}" -name "*_permutations.h" | wc -l)
echo "=== Done! Generated ${TOTAL_HEADERS} permutation headers in ${OUTPUT_DIR} ==="
