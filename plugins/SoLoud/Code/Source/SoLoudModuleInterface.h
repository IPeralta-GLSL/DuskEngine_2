#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace SoLoudGem
{
    extern AZ::ComponentDescriptor* SoLoudSystemComponent_CreateDescriptor();
    extern AZ::TypeId SoLoudSystemComponent_GetTypeId();
    extern AZ::ComponentDescriptor* SoLoudPlaybackComponent_CreateDescriptor();
    extern AZ::ComponentDescriptor* SoLoudListenerComponent_CreateDescriptor();

    class SoLoudModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(SoLoudModuleInterface, "{E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B}", AZ::Module);
        AZ_CLASS_ALLOCATOR(SoLoudModuleInterface, AZ::SystemAllocator, 0);

        SoLoudModuleInterface()
        {
            m_descriptors.insert(m_descriptors.end(), {
                SoLoudSystemComponent_CreateDescriptor(),
                SoLoudPlaybackComponent_CreateDescriptor(),
                SoLoudListenerComponent_CreateDescriptor(),
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                SoLoudSystemComponent_GetTypeId(),
            };
        }
    };
}
