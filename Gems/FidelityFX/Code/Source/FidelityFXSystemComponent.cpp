#include <FidelityFXSystemComponent.h>
#include <Fsr3Pass.h>
#include <Fsr3FeatureProcessor.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>

namespace AZ::Render
{
    void FidelityFXSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<FidelityFXSystemComponent, AZ::Component>()
                ->Version(0);
        }

        Fsr3PassData::Reflect(context);
        Fsr3FeatureProcessor::Reflect(context);
    }

    void FidelityFXSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("FidelityFXService"));
    }

    void FidelityFXSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("FidelityFXService"));
    }

    void FidelityFXSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void FidelityFXSystemComponent::Init()
    {
    }

    void FidelityFXSystemComponent::Activate()
    {
        auto* passSystem = RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");

        passSystem->AddPassCreator(Name("Fsr3Pass"), &Fsr3Pass::Create);

        RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<Fsr3FeatureProcessor>();

        m_loadTemplatesHandler = RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler(
            [this]() { this->LoadPassTemplateMappings(); });
        passSystem->ConnectEvent(m_loadTemplatesHandler);
    }

    void FidelityFXSystemComponent::Deactivate()
    {
        RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<Fsr3FeatureProcessor>();
        m_loadTemplatesHandler.Disconnect();
    }

    void FidelityFXSystemComponent::LoadPassTemplateMappings()
    {
        auto* passSystem = RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");

        const char* passTemplatesFile = "Passes/FidelityFX_PassTemplates.azasset";
        passSystem->LoadPassTemplateMappings(passTemplatesFile);
    }
}
