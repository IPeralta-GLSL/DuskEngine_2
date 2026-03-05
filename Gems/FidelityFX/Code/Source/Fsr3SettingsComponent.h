#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

namespace AZ::Render
{
    enum class Fsr3QualityMode : int
    {
        NativeAA = 0,
        Quality = 1,
        Balanced = 2,
        Performance = 3,
        UltraPerformance = 4,
    };

    class Fsr3SettingsComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(Fsr3SettingsComponent, "{1A2B3C4D-5E6F-7A8B-9C0D-E1F2A3B4C5D6}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:
        void Activate() override;
        void Deactivate() override;

        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:
        void ApplySettings();

        bool m_enabled = true;
        Fsr3QualityMode m_qualityMode = Fsr3QualityMode::Performance;
        float m_sharpness = 0.0f;

        bool m_lastEnabled = false;
        Fsr3QualityMode m_lastQualityMode = Fsr3QualityMode::NativeAA;
        float m_lastSharpness = -1.0f;
    };
}
