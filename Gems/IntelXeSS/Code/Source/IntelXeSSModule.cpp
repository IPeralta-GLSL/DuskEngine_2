#include <IntelXeSSModule.h>

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AZ::Render::IntelXeSSModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_IntelXeSS, AZ::Render::IntelXeSSModule)
#endif
