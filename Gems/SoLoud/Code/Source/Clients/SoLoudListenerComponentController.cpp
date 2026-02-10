#include "SoLoudListenerComponentController.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <soloud.h>

namespace SoLoudGem
{
    SoLoudListenerComponentController::SoLoudListenerComponentController()
    {
    }

    SoLoudListenerComponentController::SoLoudListenerComponentController(const SoLoudListenerComponentConfig& config)
        : m_config(config)
    {
    }

    void SoLoudListenerComponentController::Reflect(AZ::ReflectContext* context)
    {
        SoLoudListenerComponentConfig::Reflect(context);

        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SoLoudListenerComponentController>()
                ->Version(1)
                ->Field("Config", &SoLoudListenerComponentController::m_config);
        }
    }

    void SoLoudListenerComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SoLoudListenerService"));
    }

    void SoLoudListenerComponentController::Activate(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_entityComponentIdPair = entityComponentIdPair;
        SoLoudListenerRequestBus::Handler::BusConnect(m_entityComponentIdPair.GetEntityId());

        if (m_config.m_followEntity)
        {
            AZ::TransformBus::Event(
                m_entityComponentIdPair.GetEntityId(),
                &AZ::TransformBus::Events::BindTransformChangedEventHandler,
                m_entityMovedHandler);

            AZ::Transform worldTm = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldTm, m_entityComponentIdPair.GetEntityId(),
                &AZ::TransformBus::Events::GetWorldTM);
            OnWorldTransformChanged(worldTm);
        }

        SoLoud::Soloud* engine = SoLoudInterface::Get() ? SoLoudInterface::Get()->GetEngine() : nullptr;
        if (engine)
        {
            engine->setGlobalVolume(m_config.m_globalVolume);
        }
    }

    void SoLoudListenerComponentController::Deactivate()
    {
        m_entityMovedHandler.Disconnect();
        SoLoudListenerRequestBus::Handler::BusDisconnect();
    }

    void SoLoudListenerComponentController::SetConfiguration(const SoLoudListenerComponentConfig& config)
    {
        m_config = config;
    }

    const SoLoudListenerComponentConfig& SoLoudListenerComponentController::GetConfiguration() const
    {
        return m_config;
    }

    void SoLoudListenerComponentController::SetPosition(const AZ::Vector3& position)
    {
        m_position = position;
        UpdateListenerInEngine();
    }

    void SoLoudListenerComponentController::SetForwardAndUp(const AZ::Vector3& forward, const AZ::Vector3& up)
    {
        m_forward = forward;
        m_up = up;
        UpdateListenerInEngine();
    }

    void SoLoudListenerComponentController::SetVelocity(const AZ::Vector3& velocity)
    {
        m_velocity = velocity;
        UpdateListenerInEngine();
    }

    void SoLoudListenerComponentController::SetGlobalVolume(float volume)
    {
        m_config.m_globalVolume = volume;
        SoLoud::Soloud* engine = SoLoudInterface::Get() ? SoLoudInterface::Get()->GetEngine() : nullptr;
        if (engine)
        {
            engine->setGlobalVolume(volume);
        }
    }

    float SoLoudListenerComponentController::GetGlobalVolume() const
    {
        return m_config.m_globalVolume;
    }

    void SoLoudListenerComponentController::UpdateListenerInEngine()
    {
        SoLoud::Soloud* engine = SoLoudInterface::Get() ? SoLoudInterface::Get()->GetEngine() : nullptr;
        if (!engine)
        {
            return;
        }

        engine->set3dListenerParameters(
            m_position.GetX(), m_position.GetY(), m_position.GetZ(),
            m_forward.GetX(), m_forward.GetY(), m_forward.GetZ(),
            m_up.GetX(), m_up.GetY(), m_up.GetZ(),
            m_velocity.GetX(), m_velocity.GetY(), m_velocity.GetZ());
        engine->update3dAudio();
    }

    void SoLoudListenerComponentController::OnWorldTransformChanged(const AZ::Transform& world)
    {
        m_position = world.GetTranslation();
        m_forward = world.GetBasisY();
        m_up = world.GetBasisZ();
        UpdateListenerInEngine();
    }
}
