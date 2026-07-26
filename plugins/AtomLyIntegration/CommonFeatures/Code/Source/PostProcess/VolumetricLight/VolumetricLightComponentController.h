/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>

#include <AtomLyIntegration/CommonFeatures/PostProcess/VolumetricLight/VolumetricLightBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/VolumetricLight/VolumetricLightComponentConfig.h>

#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class VolumetricLightComponentController final
            : public VolumetricLightRequestBus::Handler
        {
        public:
            friend class EditorVolumetricLightComponent;

            AZ_TYPE_INFO(AZ::Render::VolumetricLightComponentController, "{2E3F4A5B-6C7D-8E9F-0A1B-2C3D4E5F6A7B}");
            static void Reflect(ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            VolumetricLightComponentController() = default;
            VolumetricLightComponentController(const VolumetricLightComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const VolumetricLightComponentConfig& config);
            const VolumetricLightComponentConfig& GetConfiguration() const;

#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(VolumetricLightComponentController);

            void OnConfigChanged();

            PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            VolumetricLightSettingsInterface* m_settingsInterface = nullptr;
            VolumetricLightComponentConfig m_configuration;
            EntityId m_entityId;
        };
    }
}
