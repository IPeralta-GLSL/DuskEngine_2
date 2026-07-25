/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    namespace Render
    {
        class VolumetricLightSettingsInterface
        {
        public:
            AZ_RTTI(AZ::Render::VolumetricLightSettingsInterface, "{B2E9C7D4-8A3F-4B6C-9E1D-7F5A3C2B4E8D}");

            virtual void OnSettingsChanged() = 0;

            virtual void SetInitialized(bool isInitialized) = 0;
            virtual bool IsInitialized() = 0;

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
            virtual ValueType Get##Name() const = 0;                                                    \
            virtual void Set##Name(ValueType val) = 0;                                                  \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        };
    }
}
