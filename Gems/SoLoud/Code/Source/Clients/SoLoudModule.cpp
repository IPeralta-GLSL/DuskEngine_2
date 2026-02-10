#include <SoLoudModuleInterface.h>

namespace SoLoudGem
{
    class SoLoudModule : public SoLoudModuleInterface
    {
    public:
        AZ_RTTI(SoLoudModule, "{F2A3B4C5-D6E7-8F9A-0B1C-2D3E4F5A6B7C}", SoLoudModuleInterface);
        AZ_CLASS_ALLOCATOR(SoLoudModule, AZ::SystemAllocator, 0);
    };
}

AZ_DECLARE_MODULE_CLASS(Gem_SoLoud, SoLoudGem::SoLoudModule)
