#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Clients/SoLoudPlaybackComponent.h>
#include <Clients/SoLoudPlaybackComponentController.h>
#include <ToolsComponents/EditorComponentAdapter.h>
#include <SoLoud/SoLoudConstants.h>

namespace SoLoudGem
{
    class EditorSoLoudPlaybackComponent
        : public AzToolsFramework::Components::EditorComponentAdapter<SoLoudPlaybackComponentController,
            SoLoudPlaybackComponent, SoLoudPlaybackComponentConfig>
    {
    public:
        using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<SoLoudPlaybackComponentController,
            SoLoudPlaybackComponent, SoLoudPlaybackComponentConfig>;
        AZ_EDITOR_COMPONENT(EditorSoLoudPlaybackComponent, EditorSoLoudPlaybackComponentTypeId, BaseClass);
        static void Reflect(AZ::ReflectContext* context);

        EditorSoLoudPlaybackComponent() = default;
        explicit EditorSoLoudPlaybackComponent(const SoLoudPlaybackComponentConfig& config);

        void Activate() override;
        void Deactivate() override;

        AZ::u32 OnConfigurationChanged() override;

        AZ::Crc32 PlaySoundInEditor();
        AZ::Crc32 StopSoundInEditor();
        AZ::Crc32 PauseSoundInEditor();
    };
}
