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

namespace AZ
{
    namespace Render
    {
        class VolumetricLightRequests
            : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::VolumetricLightRequests, "{9C8B7A6F-5E4D-3C2B-1A0F-9E8D7C6B5A4F}");

            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            virtual ~VolumetricLightRequests() {}

#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        };

        typedef AZ::EBus<VolumetricLightRequests> VolumetricLightRequestBus;
    }
}
