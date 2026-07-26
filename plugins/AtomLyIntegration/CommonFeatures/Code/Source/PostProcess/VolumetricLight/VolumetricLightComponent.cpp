/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <PostProcess/VolumetricLight/VolumetricLightComponent.h>

namespace AZ
{
    namespace Render
    {
        VolumetricLightComponent::VolumetricLightComponent(const VolumetricLightComponentConfig& config)
            : BaseClass(config)
        {
        }

        void VolumetricLightComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<VolumetricLightComponent, BaseClass>();
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<VolumetricLightComponent>()->RequestBus("VolumetricLightRequestBus");

                behaviorContext->ConstantProperty("VolumetricLightComponentTypeId", BehaviorConstant(Uuid(VolumetricLightComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }
    }
}
