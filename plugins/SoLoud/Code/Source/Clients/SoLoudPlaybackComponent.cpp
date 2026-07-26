#include "SoLoudPlaybackComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace SoLoudGem
{
    AZ::ComponentDescriptor* SoLoudPlaybackComponent_CreateDescriptor()
    {
        return SoLoudPlaybackComponent::CreateDescriptor();
    }

    SoLoudPlaybackComponent::SoLoudPlaybackComponent(const SoLoudPlaybackComponentConfig& config)
        : BaseClass(config)
    {
    }

    void SoLoudPlaybackComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SoLoudPlaybackComponent, BaseClass>()->Version(1);
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext
                ->ConstantProperty("SoLoudPlaybackComponentTypeId", BehaviorConstant(AZ::Uuid(SoLoudPlaybackComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "SoLoud")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);

            behaviorContext->EBus<SoLoudPlaybackRequestBus>("SoLoudPlaybackRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "audio")
                ->Attribute(AZ::Script::Attributes::Category, "SoLoud Playback")
                ->Event("Play", &SoLoudPlaybackRequests::Play)
                ->Event("Stop", &SoLoudPlaybackRequests::Stop)
                ->Event("Pause", &SoLoudPlaybackRequests::Pause)
                ->Event("SetLooping", &SoLoudPlaybackRequests::SetLooping)
                ->Event("IsLooping", &SoLoudPlaybackRequests::IsLooping)
                ->Event("SetVolume", &SoLoudPlaybackRequests::SetVolume)
                ->Event("GetVolume", &SoLoudPlaybackRequests::GetVolume)
                ->Event("SetPan", &SoLoudPlaybackRequests::SetPan)
                ->Event("GetPan", &SoLoudPlaybackRequests::GetPan)
                ->Event("SetPlaybackSpeed", &SoLoudPlaybackRequests::SetPlaybackSpeed)
                ->Event("GetPlaybackSpeed", &SoLoudPlaybackRequests::GetPlaybackSpeed)
                ->Event("Set3dPosition", &SoLoudPlaybackRequests::Set3dPosition)
                ->Event("Set3dVelocity", &SoLoudPlaybackRequests::Set3dVelocity)
                ->Event("Set3dMinMaxDistance", &SoLoudPlaybackRequests::Set3dMinMaxDistance)
                ->Event("Set3dAttenuation", &SoLoudPlaybackRequests::Set3dAttenuation);

            behaviorContext->Class<SoLoudPlaybackComponentController>()->RequestBus("SoLoudPlaybackRequestBus");
        }
    }
}
