#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

namespace AZ::Render
{
    enum class XeSSQualityMode : int
    {
        UltraPerformance = 100,
        Performance = 101,
        Balanced = 102,
        Quality = 103,
        UltraQuality = 104,
        UltraQualityPlus = 105,
        AA = 106,
    };

    class XeSSSettingsComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(XeSSSettingsComponent, "{2B3C4D5E-6F7A-8B9C-0D1E-2F3A4B5C6D7E}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:
        void Activate() override;
        void Deactivate() override;

        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:
        void ApplySettings();

        static float GetUpscaleRatio(XeSSQualityMode mode);

        bool m_enabled = true;
        XeSSQualityMode m_qualityMode = XeSSQualityMode::Performance;
        float m_sharpness = 0.0f;

        bool m_lastEnabled = false;
        XeSSQualityMode m_lastQualityMode = XeSSQualityMode::UltraPerformance; // force initial apply
        float m_lastSharpness = -1.0f;
    };
} // namespace AZ::Render
