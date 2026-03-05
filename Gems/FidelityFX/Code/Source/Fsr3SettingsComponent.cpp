#include <Fsr3SettingsComponent.h>
#include <Fsr3Pass.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ::Render
{
    void Fsr3SettingsComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Fsr3SettingsComponent, AZ::Component>()
                ->Version(0)
                ->Field("Enabled", &Fsr3SettingsComponent::m_enabled)
                ->Field("QualityMode", &Fsr3SettingsComponent::m_qualityMode)
                ->Field("Sharpness", &Fsr3SettingsComponent::m_sharpness);

            if (auto* editContext = serialize->GetEditContext())
            {
                editContext->Class<Fsr3SettingsComponent>("FSR3 Settings",
                    "Controls AMD FidelityFX Super Resolution 3 upscaling")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Graphics/PostFX")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Fsr3SettingsComponent::m_enabled,
                        "Enabled", "Enable or disable FSR3 upscaling")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Fsr3SettingsComponent::m_qualityMode,
                        "Quality Mode", "FSR3 upscaling quality preset")
                        ->EnumAttribute(Fsr3QualityMode::NativeAA, "Native AA")
                        ->EnumAttribute(Fsr3QualityMode::Quality, "Quality")
                        ->EnumAttribute(Fsr3QualityMode::Balanced, "Balanced")
                        ->EnumAttribute(Fsr3QualityMode::Performance, "Performance")
                        ->EnumAttribute(Fsr3QualityMode::UltraPerformance, "Ultra Performance")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Fsr3SettingsComponent::m_sharpness,
                        "Sharpness", "RCAS sharpening amount (0 = off, 1 = max)")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly);
            }
        }
    }

    void Fsr3SettingsComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("Fsr3SettingsService"));
    }

    void Fsr3SettingsComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("Fsr3SettingsService"));
    }

    void Fsr3SettingsComponent::Activate()
    {
        ApplySettings();
        AZ::TickBus::Handler::BusConnect();
    }

    void Fsr3SettingsComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void Fsr3SettingsComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        ApplySettings();
    }

    void Fsr3SettingsComponent::ApplySettings()
    {
        auto* passSystem = RPI::PassSystemInterface::Get();
        if (!passSystem)
        {
            return;
        }

        RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(Name("Fsr3Pass"), static_cast<const RPI::Scene*>(nullptr));
        RPI::Pass* foundPass = passSystem->FindFirstPass(passFilter);
        if (!foundPass)
        {
            return;
        }

        foundPass->SetEnabled(m_enabled);

        if (m_enabled)
        {
            auto* fsr3Pass = azrtti_cast<Fsr3Pass*>(foundPass);
            if (fsr3Pass)
            {
                fsr3Pass->SetQualityMode(static_cast<FfxFsr3UpscalerQualityMode>(static_cast<int>(m_qualityMode)));
                fsr3Pass->SetSharpness(m_sharpness);
            }
        }
    }
}
