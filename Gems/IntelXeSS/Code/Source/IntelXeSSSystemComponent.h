#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ::Render
{
    class IntelXeSSSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(IntelXeSSSystemComponent, "{F5A6B7C8-D9E0-1F2A-3B4C-5D6E7F8A9B0C}");

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
} // namespace AZ::Render