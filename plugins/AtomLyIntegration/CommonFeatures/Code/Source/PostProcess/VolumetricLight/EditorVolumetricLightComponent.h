/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/VolumetricLight/VolumetricLightComponentConstants.h>
#include <PostProcess/VolumetricLight/VolumetricLightComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorVolumetricLightComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<VolumetricLightComponentController, VolumetricLightComponent, VolumetricLightComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<VolumetricLightComponentController, VolumetricLightComponent, VolumetricLightComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorVolumetricLightComponent, EditorVolumetricLightComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorVolumetricLightComponent() = default;
            EditorVolumetricLightComponent(const VolumetricLightComponentConfig& config);

            AZ::u32 OnConfigurationChanged() override;
        };
    }
}
