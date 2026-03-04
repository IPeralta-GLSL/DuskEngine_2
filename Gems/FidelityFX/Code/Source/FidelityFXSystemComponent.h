#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ::Render
{
    class FidelityFXSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(FidelityFXSystemComponent, "{D4E5F6A7-B8C9-4D0E-A1F2-3B4C5D6E7F80}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

    protected:
        void Init() override;
        void Activate() override;
        void Deactivate() override;

    private:
        void LoadPassTemplateMappings();

        RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;
    };
}
