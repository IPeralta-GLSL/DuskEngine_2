#include <FidelityFX/host/ffx_types.h>
#include <FidelityFX/host/ffx_error.h>

struct FfxFrameGenerationConfig;

extern "C" {
FFX_API FfxErrorCode ffxSetFrameGenerationConfigToSwapchainVK(FfxFrameGenerationConfig const*)
{
    return FFX_ERROR_INVALID_ARGUMENT;
}
}
