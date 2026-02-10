#include "SoLoudPlaybackComponentController.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <SoLoud/SoLoudSoundAsset.h>

#include <soloud.h>
#include <soloud_wav.h>

namespace SoLoudGem
{
    SoLoudPlaybackComponentController::SoLoudPlaybackComponentController()
    {
    }

    SoLoudPlaybackComponentController::SoLoudPlaybackComponentController(const SoLoudPlaybackComponentConfig& config)
        : m_config(config)
    {
    }

    SoLoudPlaybackComponentController::~SoLoudPlaybackComponentController()
    {
        UnloadSound();
    }

    void SoLoudPlaybackComponentController::Reflect(AZ::ReflectContext* context)
    {
        SoLoudPlaybackComponentConfig::Reflect(context);

        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SoLoudPlaybackComponentController>()
                ->Version(1)
                ->Field("Config", &SoLoudPlaybackComponentController::m_config);
        }
    }

    void SoLoudPlaybackComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SoLoudPlaybackService"));
    }

    void SoLoudPlaybackComponentController::Activate(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_entityComponentIdPair = entityComponentIdPair;
        SoLoudPlaybackRequestBus::Handler::BusConnect(m_entityComponentIdPair.GetEntityId());

        if (m_config.m_autoFollowEntity)
        {
            AZ::TransformBus::Event(
                m_entityComponentIdPair.GetEntityId(),
                &AZ::TransformBus::Events::BindTransformChangedEventHandler,
                m_entityMovedHandler);
        }

        if (m_config.m_sound.GetId().IsValid())
        {
            m_config.m_sound.QueueLoad();
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_config.m_sound.GetId());
        }
    }

    void SoLoudPlaybackComponentController::Deactivate()
    {
        Stop();
        m_entityMovedHandler.Disconnect();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        SoLoudPlaybackRequestBus::Handler::BusDisconnect();
        UnloadSound();
    }

    void SoLoudPlaybackComponentController::SetConfiguration(const SoLoudPlaybackComponentConfig& config)
    {
        m_config = config;
    }

    const SoLoudPlaybackComponentConfig& SoLoudPlaybackComponentController::GetConfiguration() const
    {
        return m_config;
    }

    void SoLoudPlaybackComponentController::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_config.m_sound = asset;
        LoadSound();

        if (m_config.m_autoplayOnActivate)
        {
            Play();
        }
    }

    void SoLoudPlaybackComponentController::LoadSound()
    {
        auto* soundAsset = m_config.m_sound.GetAs<SoLoudSoundAsset>();
        if (!soundAsset || soundAsset->m_data.empty())
        {
            return;
        }

        m_wav = AZStd::make_unique<SoLoud::Wav>();
        SoLoud::result r = m_wav->loadMem(
            soundAsset->m_data.data(),
            static_cast<unsigned int>(soundAsset->m_data.size()),
            true, false);

        if (r != SoLoud::SO_NO_ERROR)
        {
            AZ_Error("SoLoud", false, "Failed to load sound data, error %d", r);
            m_wav.reset();
            return;
        }

        m_wav->setLooping(m_config.m_loop);
        m_wav->setVolume(m_config.m_volume);

        if (m_config.m_enable3d)
        {
            m_wav->set3dMinMaxDistance(m_config.m_minDistance, m_config.m_maxDistance);
            m_wav->set3dAttenuation(m_config.m_attenuationModel, m_config.m_attenuationRolloff);
        }
    }

    void SoLoudPlaybackComponentController::UnloadSound()
    {
        m_wav.reset();
        m_voiceHandle = 0;
        m_playing = false;
    }

    void SoLoudPlaybackComponentController::Play()
    {
        if (!m_wav)
        {
            return;
        }

        SoLoud::Soloud* engine = SoLoudInterface::Get() ? SoLoudInterface::Get()->GetEngine() : nullptr;
        if (!engine)
        {
            return;
        }

        if (m_config.m_enable3d)
        {
            AZ::Transform worldTm = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldTm, m_entityComponentIdPair.GetEntityId(),
                &AZ::TransformBus::Events::GetWorldTM);
            AZ::Vector3 pos = worldTm.GetTranslation();

            m_voiceHandle = engine->play3d(*m_wav, pos.GetX(), pos.GetY(), pos.GetZ());
        }
        else
        {
            m_voiceHandle = engine->play(*m_wav);
        }

        engine->setVolume(m_voiceHandle, m_config.m_volume);
        engine->setPan(m_voiceHandle, m_config.m_pan);
        engine->setRelativePlaySpeed(m_voiceHandle, m_config.m_playbackSpeed);
        m_playing = true;
    }

    void SoLoudPlaybackComponentController::Stop()
    {
        if (!m_playing)
        {
            return;
        }

        SoLoud::Soloud* engine = SoLoudInterface::Get() ? SoLoudInterface::Get()->GetEngine() : nullptr;
        if (engine)
        {
            engine->stop(m_voiceHandle);
        }
        m_voiceHandle = 0;
        m_playing = false;
    }

    void SoLoudPlaybackComponentController::Pause()
    {
        SoLoud::Soloud* engine = SoLoudInterface::Get() ? SoLoudInterface::Get()->GetEngine() : nullptr;
        if (engine && m_playing)
        {
            bool paused = engine->getPause(m_voiceHandle);
            engine->setPause(m_voiceHandle, !paused);
        }
    }

    void SoLoudPlaybackComponentController::SetLooping(bool loop)
    {
        m_config.m_loop = loop;
        if (m_wav)
        {
            m_wav->setLooping(loop);
        }
        SoLoud::Soloud* engine = SoLoudInterface::Get() ? SoLoudInterface::Get()->GetEngine() : nullptr;
        if (engine && m_playing)
        {
            engine->setLooping(m_voiceHandle, loop);
        }
    }

    bool SoLoudPlaybackComponentController::IsLooping() const
    {
        return m_config.m_loop;
    }

    void SoLoudPlaybackComponentController::SetVolume(float volume)
    {
        m_config.m_volume = volume;
        SoLoud::Soloud* engine = SoLoudInterface::Get() ? SoLoudInterface::Get()->GetEngine() : nullptr;
        if (engine && m_playing)
        {
            engine->setVolume(m_voiceHandle, volume);
        }
    }

    float SoLoudPlaybackComponentController::GetVolume() const
    {
        return m_config.m_volume;
    }

    void SoLoudPlaybackComponentController::SetPan(float pan)
    {
        m_config.m_pan = pan;
        SoLoud::Soloud* engine = SoLoudInterface::Get() ? SoLoudInterface::Get()->GetEngine() : nullptr;
        if (engine && m_playing)
        {
            engine->setPan(m_voiceHandle, pan);
        }
    }

    float SoLoudPlaybackComponentController::GetPan() const
    {
        return m_config.m_pan;
    }

    void SoLoudPlaybackComponentController::SetPlaybackSpeed(float speed)
    {
        m_config.m_playbackSpeed = speed;
        SoLoud::Soloud* engine = SoLoudInterface::Get() ? SoLoudInterface::Get()->GetEngine() : nullptr;
        if (engine && m_playing)
        {
            engine->setRelativePlaySpeed(m_voiceHandle, speed);
        }
    }

    float SoLoudPlaybackComponentController::GetPlaybackSpeed() const
    {
        return m_config.m_playbackSpeed;
    }

    void SoLoudPlaybackComponentController::Set3dPosition(const AZ::Vector3& position)
    {
        SoLoud::Soloud* engine = SoLoudInterface::Get() ? SoLoudInterface::Get()->GetEngine() : nullptr;
        if (engine && m_playing)
        {
            engine->set3dSourcePosition(m_voiceHandle, position.GetX(), position.GetY(), position.GetZ());
        }
    }

    void SoLoudPlaybackComponentController::Set3dVelocity(const AZ::Vector3& velocity)
    {
        SoLoud::Soloud* engine = SoLoudInterface::Get() ? SoLoudInterface::Get()->GetEngine() : nullptr;
        if (engine && m_playing)
        {
            engine->set3dSourceVelocity(m_voiceHandle, velocity.GetX(), velocity.GetY(), velocity.GetZ());
        }
    }

    void SoLoudPlaybackComponentController::Set3dMinMaxDistance(float minDist, float maxDist)
    {
        m_config.m_minDistance = minDist;
        m_config.m_maxDistance = maxDist;
        SoLoud::Soloud* engine = SoLoudInterface::Get() ? SoLoudInterface::Get()->GetEngine() : nullptr;
        if (engine && m_playing)
        {
            engine->set3dSourceMinMaxDistance(m_voiceHandle, minDist, maxDist);
        }
    }

    void SoLoudPlaybackComponentController::Set3dAttenuation(int model, float rolloff)
    {
        m_config.m_attenuationModel = model;
        m_config.m_attenuationRolloff = rolloff;
        SoLoud::Soloud* engine = SoLoudInterface::Get() ? SoLoudInterface::Get()->GetEngine() : nullptr;
        if (engine && m_playing)
        {
            engine->set3dSourceAttenuation(m_voiceHandle, static_cast<unsigned int>(model), rolloff);
        }
    }

    void SoLoudPlaybackComponentController::OnWorldTransformChanged(const AZ::Transform& world)
    {
        if (m_config.m_enable3d && m_playing)
        {
            AZ::Vector3 pos = world.GetTranslation();
            Set3dPosition(pos);
        }
    }
}
