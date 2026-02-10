#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>

namespace SoLoudGem
{
    class SoLoudListenerRequests : public AZ::ComponentBus
    {
    public:
        ~SoLoudListenerRequests() override = default;

        virtual void SetPosition(const AZ::Vector3& position) = 0;
        virtual void SetForwardAndUp(const AZ::Vector3& forward, const AZ::Vector3& up) = 0;
        virtual void SetVelocity(const AZ::Vector3& velocity) = 0;
        virtual float GetGlobalVolume() const = 0;
        virtual void SetGlobalVolume(float volume) = 0;
    };

    using SoLoudListenerRequestBus = AZ::EBus<SoLoudListenerRequests>;
}
