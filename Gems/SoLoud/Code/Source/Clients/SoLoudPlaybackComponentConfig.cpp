#include "SoLoudPlaybackComponentConfig.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetSerializer.h>

namespace SoLoudGem
{
    void SoLoudPlaybackComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SoLoudPlaybackComponentConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("Sound", &SoLoudPlaybackComponentConfig::m_sound)
                ->Field("Autoplay", &SoLoudPlaybackComponentConfig::m_autoplayOnActivate)
                ->Field("Volume", &SoLoudPlaybackComponentConfig::m_volume)
                ->Field("Loop", &SoLoudPlaybackComponentConfig::m_loop)
                ->Field("Enable3D", &SoLoudPlaybackComponentConfig::m_enable3d)
                ->Field("MinDistance", &SoLoudPlaybackComponentConfig::m_minDistance)
                ->Field("MaxDistance", &SoLoudPlaybackComponentConfig::m_maxDistance)
                ->Field("Pan", &SoLoudPlaybackComponentConfig::m_pan)
                ->Field("PlaybackSpeed", &SoLoudPlaybackComponentConfig::m_playbackSpeed)
                ->Field("AttenuationModel", &SoLoudPlaybackComponentConfig::m_attenuationModel)
                ->Field("AttenuationRolloff", &SoLoudPlaybackComponentConfig::m_attenuationRolloff)
                ->Field("AutoFollow", &SoLoudPlaybackComponentConfig::m_autoFollowEntity);
        }
    }
}
