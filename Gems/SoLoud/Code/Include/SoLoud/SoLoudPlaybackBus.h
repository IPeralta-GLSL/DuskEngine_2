#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>

namespace SoLoudGem
{
    class SoLoudPlaybackRequests : public AZ::ComponentBus
    {
    public:
        ~SoLoudPlaybackRequests() override = default;

        virtual void Play() = 0;
        virtual void Stop() = 0;
        virtual void Pause() = 0;
        virtual void SetLooping(bool loop) = 0;
        virtual bool IsLooping() const = 0;
        virtual void SetVolume(float volume) = 0;
        virtual float GetVolume() const = 0;
        virtual void SetPan(float pan) = 0;
        virtual float GetPan() const = 0;
        virtual void SetPlaybackSpeed(float speed) = 0;
        virtual float GetPlaybackSpeed() const = 0;
        virtual void Set3dPosition(const AZ::Vector3& position) = 0;
        virtual void Set3dVelocity(const AZ::Vector3& velocity) = 0;
        virtual void Set3dMinMaxDistance(float minDist, float maxDist) = 0;
        virtual void Set3dAttenuation(int model, float rolloff) = 0;
    };

    using SoLoudPlaybackRequestBus = AZ::EBus<SoLoudPlaybackRequests>;
}
