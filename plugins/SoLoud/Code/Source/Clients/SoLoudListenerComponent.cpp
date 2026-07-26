#include "SoLoudListenerComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace SoLoudGem
{
    AZ::ComponentDescriptor* SoLoudListenerComponent_CreateDescriptor()
    {
        return SoLoudListenerComponent::CreateDescriptor();
    }

    SoLoudListenerComponent::SoLoudListenerComponent(const SoLoudListenerComponentConfig& config)
        : BaseClass(config)
    {
    }

    void SoLoudListenerComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SoLoudListenerComponent, BaseClass>()->Version(1);
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext
                ->ConstantProperty("SoLoudListenerComponentTypeId", BehaviorConstant(AZ::Uuid(SoLoudListenerComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "SoLoud")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);

            behaviorContext->EBus<SoLoudListenerRequestBus>("SoLoudListenerRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "audio")
                ->Attribute(AZ::Script::Attributes::Category, "SoLoud Listener")
                ->Event("SetPosition", &SoLoudListenerRequests::SetPosition)
                ->Event("SetForwardAndUp", &SoLoudListenerRequests::SetForwardAndUp)
                ->Event("SetVelocity", &SoLoudListenerRequests::SetVelocity)
                ->Event("SetGlobalVolume", &SoLoudListenerRequests::SetGlobalVolume)
                ->Event("GetGlobalVolume", &SoLoudListenerRequests::GetGlobalVolume);

            behaviorContext->Class<SoLoudListenerComponentController>()->RequestBus("SoLoudListenerRequestBus");
        }
    }
}
