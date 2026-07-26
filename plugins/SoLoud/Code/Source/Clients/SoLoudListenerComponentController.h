#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <SoLoud/SoLoudBus.h>
#include <SoLoud/SoLoudListenerBus.h>
#include "SoLoudListenerComponentConfig.h"

namespace SoLoudGem
{
    class SoLoudListenerComponentController
        : public SoLoudListenerRequestBus::Handler
    {
        friend class EditorSoLoudListenerComponent;
    public:
        AZ_CLASS_ALLOCATOR(SoLoudListenerComponentController, AZ::SystemAllocator, 0);
        AZ_RTTI(SoLoudListenerComponentController, "{DD445566-7788-99AA-BBCC-EEFF00112233}");

        SoLoudListenerComponentController();
        explicit SoLoudListenerComponentController(const SoLoudListenerComponentConfig& config);
        ~SoLoudListenerComponentController() override = default;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        void Activate(const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Deactivate();
        void SetConfiguration(const SoLoudListenerComponentConfig& config);
        const SoLoudListenerComponentConfig& GetConfiguration() const;

        void SetPosition(const AZ::Vector3& position) override;
        void SetForwardAndUp(const AZ::Vector3& forward, const AZ::Vector3& up) override;
        void SetVelocity(const AZ::Vector3& velocity) override;
        void SetGlobalVolume(float volume) override;
        float GetGlobalVolume() const override;

    private:
        AZ::EntityComponentIdPair m_entityComponentIdPair;
        SoLoudListenerComponentConfig m_config;

        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
        AZ::Vector3 m_forward = AZ::Vector3(0.f, 1.f, 0.f);
        AZ::Vector3 m_up = AZ::Vector3(0.f, 0.f, 1.f);
        AZ::Vector3 m_velocity = AZ::Vector3::CreateZero();

        void UpdateListenerInEngine();

        void OnWorldTransformChanged(const AZ::Transform& world);
        AZ::TransformChangedEvent::Handler m_entityMovedHandler{[this](
            [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
        {
            OnWorldTransformChanged(world);
        }};
    };
}
