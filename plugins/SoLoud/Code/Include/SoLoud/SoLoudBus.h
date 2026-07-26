#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace SoLoud { class Soloud; }

namespace SoLoudGem
{
    class SoLoudRequests
    {
    public:
        AZ_RTTI(SoLoudRequests, "{E1F2A3B4-C5D6-7890-1234-56789ABCDEF0}");
        virtual ~SoLoudRequests() = default;

        virtual SoLoud::Soloud* GetEngine() = 0;
        virtual void SetGlobalVolume(float volume) = 0;
        virtual float GetGlobalVolume() const = 0;
    };

    class SoLoudBusTraits : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using SoLoudRequestBus = AZ::EBus<SoLoudRequests, SoLoudBusTraits>;
    using SoLoudInterface = AZ::Interface<SoLoudRequests>;
}
