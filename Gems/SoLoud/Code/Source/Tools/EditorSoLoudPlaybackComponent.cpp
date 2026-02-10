#include "EditorSoLoudPlaybackComponent.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>

namespace SoLoudGem
{
    AZ::ComponentDescriptor* EditorSoLoudPlaybackComponent_CreateDescriptor()
    {
        return EditorSoLoudPlaybackComponent::CreateDescriptor();
    }

    void EditorSoLoudPlaybackComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSoLoudPlaybackComponent, BaseClass>()->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorSoLoudPlaybackComponent>("SoLoud Playback", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "SoLoud")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->UIElement(AZ::Edit::UIHandlers::Button, "Play Sound", "")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Play Sound")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSoLoudPlaybackComponent::PlaySoundInEditor)

                    ->UIElement(AZ::Edit::UIHandlers::Button, "Stop Sound", "")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Stop Sound")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSoLoudPlaybackComponent::StopSoundInEditor)

                    ->UIElement(AZ::Edit::UIHandlers::Button, "Pause Sound", "")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Pause Sound")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSoLoudPlaybackComponent::PauseSoundInEditor);

                editContext->Class<SoLoudPlaybackComponentController>("SoLoudPlaybackComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SoLoudPlaybackComponentController::m_config, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                editContext
                    ->Class<SoLoudPlaybackComponentConfig>("SoLoud Playback Config", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &SoLoudPlaybackComponentConfig::m_sound, "Sound Asset", "")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Configuration")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &SoLoudPlaybackComponentConfig::m_autoplayOnActivate, "Autoplay", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SoLoudPlaybackComponentConfig::m_loop, "Loop", "")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SoLoudPlaybackComponentConfig::m_volume, "Volume", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SoLoudPlaybackComponentConfig::m_pan, "Pan", "")
                    ->Attribute(AZ::Edit::Attributes::Min, -1.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SoLoudPlaybackComponentConfig::m_playbackSpeed, "Playback Speed", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->Attribute(AZ::Edit::Attributes::Max, 4.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &SoLoudPlaybackComponentConfig::m_autoFollowEntity, "Auto-follow", "")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "3D Audio")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &SoLoudPlaybackComponentConfig::m_enable3d, "Enable 3D", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SoLoudPlaybackComponentConfig::m_minDistance, "Min Distance", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SoLoudPlaybackComponentConfig::m_maxDistance, "Max Distance", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SoLoudPlaybackComponentConfig::m_attenuationModel, "Attenuation Model", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SoLoudPlaybackComponentConfig::m_attenuationRolloff, "Attenuation Rolloff", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f);
            }
        }
    }

    EditorSoLoudPlaybackComponent::EditorSoLoudPlaybackComponent(const SoLoudPlaybackComponentConfig& config)
        : BaseClass(config)
    {
    }

    void EditorSoLoudPlaybackComponent::Activate()
    {
        BaseClass::Activate();
    }

    void EditorSoLoudPlaybackComponent::Deactivate()
    {
        BaseClass::Deactivate();
    }

    AZ::u32 EditorSoLoudPlaybackComponent::OnConfigurationChanged()
    {
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorSoLoudPlaybackComponent::PlaySoundInEditor()
    {
        m_controller.Play();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorSoLoudPlaybackComponent::StopSoundInEditor()
    {
        m_controller.Stop();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorSoLoudPlaybackComponent::PauseSoundInEditor()
    {
        m_controller.Pause();
        return AZ::Edit::PropertyRefreshLevels::None;
    }
}
