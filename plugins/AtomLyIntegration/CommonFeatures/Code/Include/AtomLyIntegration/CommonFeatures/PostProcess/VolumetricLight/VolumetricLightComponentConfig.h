/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/VolumetricLight/VolumetricLightComponentConstants.h>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightSettingsInterface.h>

namespace AZ
{
    namespace Render
    {
        class VolumetricLightComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(VolumetricLightComponentConfig, SystemAllocator)
            AZ_RTTI(AZ::Render::VolumetricLightComponentConfig, "{1D2E3F4A-5B6C-7D8E-9F0A-1B2C3D4E5F6A}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void CopySettingsFrom(VolumetricLightSettingsInterface* settings);
            void CopySettingsTo(VolumetricLightSettingsInterface* settings);

            bool ArePropertiesReadOnly() const { return !m_enabled; }
        };
    }
}
