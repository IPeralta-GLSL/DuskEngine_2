#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <SoLoud/SoLoudConstants.h>
#include <SoLoud/SoLoudPlaybackBus.h>
#include "SoLoudPlaybackComponentController.h"
#include "SoLoudPlaybackComponentConfig.h"

namespace SoLoudGem
{
    class SoLoudPlaybackComponent final
        : public AzFramework::Components::ComponentAdapter<SoLoudPlaybackComponentController, SoLoudPlaybackComponentConfig>
    {
        using BaseClass = AzFramework::Components::ComponentAdapter<SoLoudPlaybackComponentController, SoLoudPlaybackComponentConfig>;
    public:
        AZ_COMPONENT(SoLoudPlaybackComponent, SoLoudPlaybackComponentTypeId, BaseClass);
        SoLoudPlaybackComponent() = default;
        explicit SoLoudPlaybackComponent(const SoLoudPlaybackComponentConfig& config);

        static void Reflect(AZ::ReflectContext* context);
    };
}
