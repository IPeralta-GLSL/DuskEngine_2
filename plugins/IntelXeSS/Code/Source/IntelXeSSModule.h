#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <IntelXeSSSystemComponent.h>
#include <XeSSSettingsComponent.h>

namespace AZ::Render
{
    class IntelXeSSModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(IntelXeSSModule, "{E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B}", AZ::Module);
        AZ_CLASS_ALLOCATOR(IntelXeSSModule, AZ::SystemAllocator);

        IntelXeSSModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                IntelXeSSSystemComponent::CreateDescriptor(),
                XeSSSettingsComponent::CreateDescriptor(),
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<IntelXeSSSystemComponent>()
            };
        }
    };
} // namespace AZ::Render
