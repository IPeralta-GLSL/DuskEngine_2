#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/Component.h>

namespace SoLoudGem
{
    class SoLoudListenerComponentConfig final : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(SoLoudListenerComponentConfig, "{CC334455-6677-8899-AABB-DDEEFF001122}");

        static void Reflect(AZ::ReflectContext* context);

        bool m_followEntity = true;
        float m_globalVolume = 1.0f;
    };
}
