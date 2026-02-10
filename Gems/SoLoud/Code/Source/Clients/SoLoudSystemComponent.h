#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Asset/AssetManager.h>
#include <SoLoud/SoLoudBus.h>

namespace SoLoud { class Soloud; }

namespace SoLoudGem
{
    class SoLoudSystemComponent
        : public AZ::Component
        , public SoLoudRequestBus::Handler
    {
    public:
        AZ_COMPONENT(SoLoudSystemComponent, "{11223344-5566-7788-99AA-BBCCDDEEFF00}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        SoLoudSystemComponent();
        SoLoudSystemComponent(const SoLoudSystemComponent&) = delete;
        SoLoudSystemComponent& operator=(const SoLoudSystemComponent&) = delete;
        ~SoLoudSystemComponent() override;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        SoLoud::Soloud* GetEngine() override;
        void SetGlobalVolume(float volume) override;
        float GetGlobalVolume() const override;

    private:
        AZStd::unique_ptr<SoLoud::Soloud> m_soloud;
        float m_globalVolume = 1.f;
        bool m_initialized = false;
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
    };
}
