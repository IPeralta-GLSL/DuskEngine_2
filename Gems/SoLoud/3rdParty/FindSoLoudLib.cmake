set(SOLOUD_ROOT ${CMAKE_CURRENT_LIST_DIR}/soloud)

set(SOLOUD_CORE_SOURCES
    ${SOLOUD_ROOT}/src/core/soloud.cpp
    ${SOLOUD_ROOT}/src/core/soloud_audiosource.cpp
    ${SOLOUD_ROOT}/src/core/soloud_bus.cpp
    ${SOLOUD_ROOT}/src/core/soloud_core_3d.cpp
    ${SOLOUD_ROOT}/src/core/soloud_core_basicops.cpp
    ${SOLOUD_ROOT}/src/core/soloud_core_faderops.cpp
    ${SOLOUD_ROOT}/src/core/soloud_core_filterops.cpp
    ${SOLOUD_ROOT}/src/core/soloud_core_getters.cpp
    ${SOLOUD_ROOT}/src/core/soloud_core_setters.cpp
    ${SOLOUD_ROOT}/src/core/soloud_core_voicegroup.cpp
    ${SOLOUD_ROOT}/src/core/soloud_core_voiceops.cpp
    ${SOLOUD_ROOT}/src/core/soloud_fader.cpp
    ${SOLOUD_ROOT}/src/core/soloud_fft.cpp
    ${SOLOUD_ROOT}/src/core/soloud_fft_lut.cpp
    ${SOLOUD_ROOT}/src/core/soloud_file.cpp
    ${SOLOUD_ROOT}/src/core/soloud_filter.cpp
    ${SOLOUD_ROOT}/src/core/soloud_misc.cpp
    ${SOLOUD_ROOT}/src/core/soloud_queue.cpp
    ${SOLOUD_ROOT}/src/core/soloud_thread.cpp
)

set(SOLOUD_AUDIOSOURCE_SOURCES
    ${SOLOUD_ROOT}/src/audiosource/wav/soloud_wav.cpp
    ${SOLOUD_ROOT}/src/audiosource/wav/soloud_wavstream.cpp
    ${SOLOUD_ROOT}/src/audiosource/wav/stb_vorbis.c
    ${SOLOUD_ROOT}/src/audiosource/wav/dr_impl.cpp
    ${SOLOUD_ROOT}/src/audiosource/speech/soloud_speech.cpp
    ${SOLOUD_ROOT}/src/audiosource/speech/darray.cpp
    ${SOLOUD_ROOT}/src/audiosource/speech/klatt.cpp
    ${SOLOUD_ROOT}/src/audiosource/speech/resonator.cpp
    ${SOLOUD_ROOT}/src/audiosource/speech/tts.cpp
    ${SOLOUD_ROOT}/src/audiosource/sfxr/soloud_sfxr.cpp
    ${SOLOUD_ROOT}/src/audiosource/noise/soloud_noise.cpp
)

set(SOLOUD_FILTER_SOURCES
    ${SOLOUD_ROOT}/src/filter/soloud_bassboostfilter.cpp
    ${SOLOUD_ROOT}/src/filter/soloud_biquadresonantfilter.cpp
    ${SOLOUD_ROOT}/src/filter/soloud_dcremovalfilter.cpp
    ${SOLOUD_ROOT}/src/filter/soloud_duckfilter.cpp
    ${SOLOUD_ROOT}/src/filter/soloud_echofilter.cpp
    ${SOLOUD_ROOT}/src/filter/soloud_eqfilter.cpp
    ${SOLOUD_ROOT}/src/filter/soloud_fftfilter.cpp
    ${SOLOUD_ROOT}/src/filter/soloud_flangerfilter.cpp
    ${SOLOUD_ROOT}/src/filter/soloud_freeverbfilter.cpp
    ${SOLOUD_ROOT}/src/filter/soloud_lofifilter.cpp
    ${SOLOUD_ROOT}/src/filter/soloud_robotizefilter.cpp
    ${SOLOUD_ROOT}/src/filter/soloud_waveshaperfilter.cpp
)

set(SOLOUD_BACKEND_SOURCES "")

if(${PAL_PLATFORM_NAME} STREQUAL "Linux")
    set(SOLOUD_BACKEND_SOURCES ${SOLOUD_ROOT}/src/backend/miniaudio/soloud_miniaudio.cpp)
    set(SOLOUD_BACKEND_DEFINE WITH_MINIAUDIO)
elseif(${PAL_PLATFORM_NAME} STREQUAL "Windows")
    set(SOLOUD_BACKEND_SOURCES ${SOLOUD_ROOT}/src/backend/wasapi/soloud_wasapi.cpp)
    set(SOLOUD_BACKEND_DEFINE WITH_WASAPI)
elseif(${PAL_PLATFORM_NAME} STREQUAL "Mac")
    set(SOLOUD_BACKEND_SOURCES ${SOLOUD_ROOT}/src/backend/coreaudio/soloud_coreaudio.cpp)
    set(SOLOUD_BACKEND_DEFINE WITH_COREAUDIO)
elseif(${PAL_PLATFORM_NAME} STREQUAL "Android")
    set(SOLOUD_BACKEND_SOURCES ${SOLOUD_ROOT}/src/backend/opensles/soloud_opensles.cpp)
    set(SOLOUD_BACKEND_DEFINE WITH_OPENSLES)
elseif(${PAL_PLATFORM_NAME} STREQUAL "iOS")
    set(SOLOUD_BACKEND_SOURCES ${SOLOUD_ROOT}/src/backend/coreaudio/soloud_coreaudio.cpp)
    set(SOLOUD_BACKEND_DEFINE WITH_COREAUDIO)
else()
    set(SOLOUD_BACKEND_SOURCES ${SOLOUD_ROOT}/src/backend/miniaudio/soloud_miniaudio.cpp)
    set(SOLOUD_BACKEND_DEFINE WITH_MINIAUDIO)
endif()

add_library(SoLoudLib_internal STATIC
    ${SOLOUD_CORE_SOURCES}
    ${SOLOUD_AUDIOSOURCE_SOURCES}
    ${SOLOUD_FILTER_SOURCES}
    ${SOLOUD_BACKEND_SOURCES}
)

target_include_directories(SoLoudLib_internal
    PUBLIC
        ${SOLOUD_ROOT}/include
    PRIVATE
        ${SOLOUD_ROOT}/src
)

target_compile_definitions(SoLoudLib_internal
    PUBLIC
        ${SOLOUD_BACKEND_DEFINE}
)

target_compile_options(SoLoudLib_internal PRIVATE
    -Wno-misleading-indentation
    -Wno-missing-braces
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-sign-compare
    -Wno-implicit-fallthrough
    -Wno-deprecated-declarations
    -Wno-missing-field-initializers
    -Wno-error
)

set_target_properties(SoLoudLib_internal PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(NOT TARGET 3rdParty::SoLoudLib)
    add_library(3rdParty::SoLoudLib ALIAS SoLoudLib_internal)
endif()

set(SoLoudLib_FOUND TRUE)
