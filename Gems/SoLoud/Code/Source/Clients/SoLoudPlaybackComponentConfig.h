#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>
#include <SoLoud/SoLoudSoundAsset.h>

namespace SoLoudGem
{
    class SoLoudPlaybackComponentConfig final : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(SoLoudPlaybackComponentConfig, "{AA112233-4455-6677-8899-AABBCCDDEEF0}");

        static void Reflect(AZ::ReflectContext* context);

        AZ::Data::Asset<SoLoudSoundAsset> m_sound;
        bool m_autoplayOnActivate = false;
        float m_volume = 1.0f;
        bool m_loop = false;
        bool m_enable3d = false;
        float m_minDistance = 1.f;
        float m_maxDistance = 100.f;
        float m_pan = 0.f;
        float m_playbackSpeed = 1.f;
        int m_attenuationModel = 1;
        float m_attenuationRolloff = 1.f;
        bool m_autoFollowEntity = false;
    };
}
