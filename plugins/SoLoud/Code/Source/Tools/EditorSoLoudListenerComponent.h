#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Clients/SoLoudListenerComponent.h>
#include <Clients/SoLoudListenerComponentController.h>
#include <ToolsComponents/EditorComponentAdapter.h>
#include <SoLoud/SoLoudConstants.h>

namespace SoLoudGem
{
    class EditorSoLoudListenerComponent
        : public AzToolsFramework::Components::EditorComponentAdapter<SoLoudListenerComponentController,
            SoLoudListenerComponent, SoLoudListenerComponentConfig>
    {
    public:
        using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<SoLoudListenerComponentController,
            SoLoudListenerComponent, SoLoudListenerComponentConfig>;
        AZ_EDITOR_COMPONENT(EditorSoLoudListenerComponent, EditorSoLoudListenerComponentTypeId, BaseClass);
        static void Reflect(AZ::ReflectContext* context);

        EditorSoLoudListenerComponent() = default;
        explicit EditorSoLoudListenerComponent(const SoLoudListenerComponentConfig& config);

        void Activate() override;
        void Deactivate() override;

        AZ::u32 OnConfigurationChanged() override;
    };
}
