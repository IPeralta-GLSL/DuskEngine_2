/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AtomLyIntegration/CommonFeatures/PostProcess/VolumetricLight/VolumetricLightComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        void VolumetricLightComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<VolumetricLightComponentConfig, ComponentConfig>()
                    ->Version(1)

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
                    ->Field(#Name, &VolumetricLightComponentConfig::MemberName)

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

                    ;
            }
        }

        void VolumetricLightComponentConfig::CopySettingsFrom(VolumetricLightSettingsInterface* settings)
        {
            if (settings)
            {
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
                MemberName = settings->Get##Name();

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
            }
        }

        void VolumetricLightComponentConfig::CopySettingsTo(VolumetricLightSettingsInterface* settings)
        {
            if (settings)
            {
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
                settings->Set##Name(MemberName);

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
            }
        }
    }
}
