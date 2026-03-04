#include <FidelityFXModule.h>

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AZ::Render::FidelityFXModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_FidelityFX, AZ::Render::FidelityFXModule)
#endif
