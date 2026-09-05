#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>

typedef struct RENDERDOC_API_1_7_0 RENDERDOC_API_1_7_0;

namespace AZ::Render
{
    class RenderDocToolsSystemComponent
        : public AZ::Component
        , protected AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(RenderDocToolsSystemComponent, "{E4C9A31B-7F2D-4B68-9A15-2D8F6C3E7B41}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        RenderDocToolsSystemComponent() = default;
        ~RenderDocToolsSystemComponent() override = default;

    protected:
        void Activate() override;
        void Deactivate() override;

        void OnWidgetActionRegistrationHook() override;
        void OnToolBarBindingHook() override;

    private:
        void LoadRenderDocLibrary();

        RENDERDOC_API_1_7_0* m_renderdocApi = nullptr;
        bool m_libraryLoadAttempted = false;
    };
}
