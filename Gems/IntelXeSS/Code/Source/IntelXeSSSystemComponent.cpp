#include <IntelXeSSSystemComponent.h>
#include <XeSSPass.h>
#include <XeSSFeatureProcessor.h>
#include <XeSSSettingsComponent.h>
#include <XeSSLoader.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>

namespace AZ::Render
{
    void IntelXeSSSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<IntelXeSSSystemComponent, AZ::Component>()
                ->Version(0);
        }

        XeSSPassData::Reflect(context);
        XeSSFeatureProcessor::Reflect(context);
        XeSSSettingsComponent::Reflect(context);
    }

    void IntelXeSSSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("IntelXeSSService"));
    }

    void IntelXeSSSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("IntelXeSSService"));
    }

    void IntelXeSSSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void IntelXeSSSystemComponent::Init()
    {
    }

    void IntelXeSSSystemComponent::Activate()
    {
        XeSSLoader::Get().Initialize();

        auto* passSystem = RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");

        passSystem->AddPassCreator(Name("XeSSPass"), &XeSSPass::Create);

        RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<XeSSFeatureProcessor>();

        m_loadTemplatesHandler = RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler(
            [this]() { this->LoadPassTemplateMappings(); });
        passSystem->ConnectEvent(m_loadTemplatesHandler);
    }

    void IntelXeSSSystemComponent::Deactivate()
    {
        RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<XeSSFeatureProcessor>();
        m_loadTemplatesHandler.Disconnect();
        XeSSLoader::Get().Shutdown();
    }

    void IntelXeSSSystemComponent::LoadPassTemplateMappings()
    {
        auto* passSystem = RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");

        const char* passTemplatesFile = "Passes/IntelXeSS_PassTemplates.azasset";
        passSystem->LoadPassTemplateMappings(passTemplatesFile);
    }
} // namespace AZ::Render