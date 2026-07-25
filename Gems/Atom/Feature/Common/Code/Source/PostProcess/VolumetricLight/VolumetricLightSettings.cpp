/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/VolumetricLight/VolumetricLightSettings.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        VolumetricLightSettings::VolumetricLightSettings(PostProcessFeatureProcessor* featureProcessor)
            : PostProcessBase(featureProcessor)
        {
        }

        VolumetricLightSettings::VolumetricLightSettings()
            : PostProcessBase(nullptr)
        {
        }

        void VolumetricLightSettings::OnSettingsChanged()
        {
            m_needUpdate = true;
            if (m_parentSettings)
            {
                m_parentSettings->OnConfigChanged();
            }
        }

        void VolumetricLightSettings::ApplySettingsTo(VolumetricLightSettings* target, float alpha) const
        {
            AZ_UNUSED(alpha);
            AZ_Assert(target != nullptr, "VolumetricLightSettings::ApplySettingsTo - target is null");

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
            target->MemberName = MemberName;                                                                \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            target->OnSettingsChanged();
        }

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType VolumetricLightSettings::Get##Name() const                                                \
        {                                                                                                   \
            return MemberName;                                                                              \
        }                                                                                                   \
        void VolumetricLightSettings::Set##Name(ValueType val)                                              \
        {                                                                                                   \
            MemberName = val;                                                                               \
            OnSettingsChanged();                                                                            \
        }                                                                                                   \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
    }
}
