#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <SoLoud/SoLoudBus.h>
#include <SoLoud/SoLoudPlaybackBus.h>
#include "SoLoudPlaybackComponentConfig.h"

namespace SoLoud { class Wav; }

namespace SoLoudGem
{
    class SoLoudPlaybackComponentController
        : public SoLoudPlaybackRequestBus::Handler
        , public AZ::Data::AssetBus::MultiHandler
    {
        friend class EditorSoLoudPlaybackComponent;
    public:
        AZ_CLASS_ALLOCATOR(SoLoudPlaybackComponentController, AZ::SystemAllocator, 0);
        AZ_RTTI(SoLoudPlaybackComponentController, "{BB223344-5566-7788-99AA-CCDDEE001122}");

        SoLoudPlaybackComponentController();
        explicit SoLoudPlaybackComponentController(const SoLoudPlaybackComponentConfig& config);
        ~SoLoudPlaybackComponentController() override;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        void Activate(const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Deactivate();
        void SetConfiguration(const SoLoudPlaybackComponentConfig& config);
        const SoLoudPlaybackComponentConfig& GetConfiguration() const;

        void Play() override;
        void Stop() override;
        void Pause() override;
        void SetLooping(bool loop) override;
        bool IsLooping() const override;
        void SetVolume(float volume) override;
        float GetVolume() const override;
        void SetPan(float pan) override;
        float GetPan() const override;
        void SetPlaybackSpeed(float speed) override;
        float GetPlaybackSpeed() const override;
        void Set3dPosition(const AZ::Vector3& position) override;
        void Set3dVelocity(const AZ::Vector3& velocity) override;
        void Set3dMinMaxDistance(float minDist, float maxDist) override;
        void Set3dAttenuation(int model, float rolloff) override;

        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

    private:
        AZ::EntityComponentIdPair m_entityComponentIdPair;
        SoLoudPlaybackComponentConfig m_config;

        void OnWorldTransformChanged(const AZ::Transform& world);
        AZ::TransformChangedEvent::Handler m_entityMovedHandler{[this](
            [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
        {
            OnWorldTransformChanged(world);
        }};

        void LoadSound();
        void UnloadSound();

        AZStd::unique_ptr<SoLoud::Wav> m_wav;
        unsigned int m_voiceHandle = 0;
        bool m_playing = false;
    };
}
