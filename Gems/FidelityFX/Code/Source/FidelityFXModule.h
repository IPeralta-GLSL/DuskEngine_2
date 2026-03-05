#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <FidelityFXSystemComponent.h>
#include <Fsr3SettingsComponent.h>

namespace AZ::Render
{
    class FidelityFXModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(FidelityFXModule, "{A3F2C8D1-5E7B-4A9D-B6C0-1D2E3F4A5B6C}", AZ::Module);
        AZ_CLASS_ALLOCATOR(FidelityFXModule, AZ::SystemAllocator);

        FidelityFXModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                FidelityFXSystemComponent::CreateDescriptor(),
                Fsr3SettingsComponent::CreateDescriptor(),
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<FidelityFXSystemComponent>()
            };
        }
    };
}
