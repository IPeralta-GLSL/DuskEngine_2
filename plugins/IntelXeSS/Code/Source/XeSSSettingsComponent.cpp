#include <XeSSSettingsComponent.h>
#include <XeSSPass.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

#include <xess/xess.h>

namespace AZ::Render
{
    void XeSSSettingsComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<XeSSSettingsComponent, AZ::Component>()
                ->Version(0)
                ->Field("Enabled", &XeSSSettingsComponent::m_enabled)
                ->Field("QualityMode", &XeSSSettingsComponent::m_qualityMode)
                ->Field("Sharpness", &XeSSSettingsComponent::m_sharpness);

            if (auto* editContext = serialize->GetEditContext())
            {
                editContext->Class<XeSSSettingsComponent>("Intel XeSS Settings",
                    "Controls Intel XeSS Super Sampling upscaling")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Graphics/PostFX")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XeSSSettingsComponent::m_enabled,
                        "Enabled", "Enable or disable Intel XeSS upscaling")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &XeSSSettingsComponent::m_qualityMode,
                        "Quality Mode", "XeSS upscaling quality preset")
                        ->EnumAttribute(XeSSQualityMode::UltraPerformance, "Ultra Performance (3.0x)")
                        ->EnumAttribute(XeSSQualityMode::Performance, "Performance (2.0x)")
                        ->EnumAttribute(XeSSQualityMode::Balanced, "Balanced (1.7x)")
                        ->EnumAttribute(XeSSQualityMode::Quality, "Quality (1.5x)")
                        ->EnumAttribute(XeSSQualityMode::UltraQuality, "Ultra Quality (1.5x)")
                        ->EnumAttribute(XeSSQualityMode::UltraQualityPlus, "Ultra Quality+ (1.3x)")
                        ->EnumAttribute(XeSSQualityMode::AA, "Anti-Aliasing (1.0x)")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &XeSSSettingsComponent::m_sharpness,
                        "Sharpness", "Sharpening amount (0 = off, 1 = max)")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly);
            }
        }
    }

    void XeSSSettingsComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("XeSSSettingsService"));
    }

    void XeSSSettingsComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("XeSSSettingsService"));
    }

    void XeSSSettingsComponent::Activate()
    {
        ApplySettings();
        AZ::TickBus::Handler::BusConnect();
    }

    void XeSSSettingsComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();

        // Restore native resolution
        AzFramework::NativeWindowHandle windowHandle = nullptr;
        AzFramework::WindowSystemRequestBus::BroadcastResult(
            windowHandle, &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);

        if (windowHandle)
        {
            AzFramework::WindowSize clientSize{};
            AzFramework::WindowRequestBus::EventResult(
                clientSize, windowHandle, &AzFramework::WindowRequestBus::Events::GetClientAreaSize);
            AzFramework::WindowRequestBus::Event(
                windowHandle, &AzFramework::WindowRequestBus::Events::SetRenderResolution, clientSize);
        }

        // Disable the pass
        auto* passSystem = RPI::PassSystemInterface::Get();
        if (passSystem)
        {
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(
                Name("XeSSPass"), static_cast<const RPI::Scene*>(nullptr));
            RPI::Pass* foundPass = passSystem->FindFirstPass(passFilter);
            if (foundPass)
            {
                foundPass->SetEnabled(false);
            }
        }
    }

    void XeSSSettingsComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        ApplySettings();
    }

    float XeSSSettingsComponent::GetUpscaleRatio(XeSSQualityMode mode)
    {
        switch (mode)
        {
        case XeSSQualityMode::AA:                 return 1.0f;
        case XeSSQualityMode::UltraQualityPlus:   return 1.3f;
        case XeSSQualityMode::UltraQuality:       return 1.5f;
        case XeSSQualityMode::Quality:             return 1.5f;
        case XeSSQualityMode::Balanced:            return 1.7f;
        case XeSSQualityMode::Performance:         return 2.0f;
        case XeSSQualityMode::UltraPerformance:    return 3.0f;
        default:                                   return 1.0f;
        }
    }

    void XeSSSettingsComponent::ApplySettings()
    {
        if (m_enabled == m_lastEnabled &&
            m_qualityMode == m_lastQualityMode &&
            m_sharpness == m_lastSharpness)
        {
            return;
        }

        auto* passSystem = RPI::PassSystemInterface::Get();
        if (!passSystem)
        {
            return;
        }

        RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(
            Name("XeSSPass"), static_cast<const RPI::Scene*>(nullptr));
        RPI::Pass* foundPass = passSystem->FindFirstPass(passFilter);
        if (!foundPass)
        {
            return;
        }

        foundPass->SetEnabled(m_enabled);

        AzFramework::NativeWindowHandle windowHandle = nullptr;
        AzFramework::WindowSystemRequestBus::BroadcastResult(
            windowHandle, &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);

        if (m_enabled)
        {
            auto* xessPass = azrtti_cast<XeSSPass*>(foundPass);
            if (xessPass)
            {
                xessPass->SetQualityMode(
                    static_cast<xess_quality_settings_t>(static_cast<int>(m_qualityMode)));
                xessPass->SetSharpness(m_sharpness);
            }

            if (windowHandle && (m_qualityMode != m_lastQualityMode || m_enabled != m_lastEnabled))
            {
                AzFramework::WindowSize clientSize{};
                AzFramework::WindowRequestBus::EventResult(
                    clientSize, windowHandle, &AzFramework::WindowRequestBus::Events::GetClientAreaSize);

                float ratio = GetUpscaleRatio(m_qualityMode);
                AzFramework::WindowSize renderSize{
                    AZStd::max(1u, static_cast<uint32_t>(clientSize.m_width / ratio)),
                    AZStd::max(1u, static_cast<uint32_t>(clientSize.m_height / ratio))
                };
                AzFramework::WindowRequestBus::Event(
                    windowHandle, &AzFramework::WindowRequestBus::Events::SetRenderResolution, renderSize);
            }
        }
        else
        {
            if (windowHandle && m_enabled != m_lastEnabled)
            {
                AzFramework::WindowSize clientSize{};
                AzFramework::WindowRequestBus::EventResult(
                    clientSize, windowHandle, &AzFramework::WindowRequestBus::Events::GetClientAreaSize);
                AzFramework::WindowRequestBus::Event(
                    windowHandle, &AzFramework::WindowRequestBus::Events::SetRenderResolution, clientSize);
            }
        }

        m_lastEnabled = m_enabled;
        m_lastQualityMode = m_qualityMode;
        m_lastSharpness = m_sharpness;
    }
} // namespace AZ::Render
