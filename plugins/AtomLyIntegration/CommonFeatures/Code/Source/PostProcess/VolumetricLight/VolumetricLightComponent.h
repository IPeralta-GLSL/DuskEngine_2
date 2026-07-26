/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/VolumetricLight/VolumetricLightComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/VolumetricLight/VolumetricLightComponentConstants.h>
#include <PostProcess/VolumetricLight/VolumetricLightComponentController.h>

namespace AZ
{
    namespace Render
    {
        class VolumetricLightComponent final
            : public AzFramework::Components::ComponentAdapter<VolumetricLightComponentController, VolumetricLightComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<VolumetricLightComponentController, VolumetricLightComponentConfig>;
            AZ_COMPONENT(AZ::Render::VolumetricLightComponent, VolumetricLightComponentTypeId, BaseClass);

            VolumetricLightComponent() = default;
            VolumetricLightComponent(const VolumetricLightComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    }
}
