#include "SoLoudListenerComponentConfig.h"
#include <AzCore/Serialization/SerializeContext.h>

namespace SoLoudGem
{
    void SoLoudListenerComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SoLoudListenerComponentConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("FollowEntity", &SoLoudListenerComponentConfig::m_followEntity)
                ->Field("GlobalVolume", &SoLoudListenerComponentConfig::m_globalVolume);
        }
    }
}
