#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <AzCore/RTTI/RTTI.h>

#include "RenderDocToolsSystemComponent.h"

namespace AZ::Render
{
    class RenderDocToolsModule : public AZ::Module
    {
    public:
        AZ_RTTI(RenderDocToolsModule, "{7A2F6D94-C1B8-4E53-B7D2-93A5F0E8C466}", AZ::Module);
        AZ_CLASS_ALLOCATOR(RenderDocToolsModule, AZ::SystemAllocator);

        RenderDocToolsModule()
            : AZ::Module()
        {
            m_descriptors.insert(
                m_descriptors.end(),
                {
                    RenderDocToolsSystemComponent::CreateDescriptor(),
                });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<RenderDocToolsSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AZ::Render::RenderDocToolsModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_RenderDocTools, AZ::Render::RenderDocToolsModule)
#endif
