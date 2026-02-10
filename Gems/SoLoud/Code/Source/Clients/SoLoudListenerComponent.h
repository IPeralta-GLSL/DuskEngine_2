#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <SoLoud/SoLoudConstants.h>
#include <SoLoud/SoLoudListenerBus.h>
#include "SoLoudListenerComponentController.h"
#include "SoLoudListenerComponentConfig.h"

namespace SoLoudGem
{
    class SoLoudListenerComponent final
        : public AzFramework::Components::ComponentAdapter<SoLoudListenerComponentController, SoLoudListenerComponentConfig>
    {
        using BaseClass = AzFramework::Components::ComponentAdapter<SoLoudListenerComponentController, SoLoudListenerComponentConfig>;
    public:
        AZ_COMPONENT(SoLoudListenerComponent, SoLoudListenerComponentTypeId, BaseClass);
        SoLoudListenerComponent() = default;
        explicit SoLoudListenerComponent(const SoLoudListenerComponentConfig& config);

        static void Reflect(AZ::ReflectContext* context);
    };
}
