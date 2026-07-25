/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <PostProcess/VolumetricLight/EditorVolumetricLightComponent.h>

namespace AZ
{
    namespace Render
    {
        void EditorVolumetricLightComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorVolumetricLightComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorVolumetricLightComponent>(
                        "Volumetric Light", "Adds directional light shafts / sun rays to the scene.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Graphics/Environment")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Post.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Post.svg")
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/components/reference/atom/volumetric-light/")
                        ;

                    editContext->Class<VolumetricLightComponentController>(
                        "VolumetricLightComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &VolumetricLightComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<VolumetricLightComponentConfig>("VolumetricLightComponentConfig", "")
                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &VolumetricLightComponentConfig::m_enabled,
                            "Enable",
                            "Enable volumetric light shafts.")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(Edit::UIHandlers::Color,
                            &VolumetricLightComponentConfig::m_lightColor,
                            "Light Color",
                            "Color tint of the light shafts.")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &VolumetricLightComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(Edit::UIHandlers::Slider, &VolumetricLightComponentConfig::m_lightIntensity, "Light Intensity", "Intensity multiplier for the shafts.")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 10.0f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &VolumetricLightComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(Edit::UIHandlers::Slider, &VolumetricLightComponentConfig::m_density, "Density", "Global density of the participating medium.")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &VolumetricLightComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(Edit::UIHandlers::Slider, &VolumetricLightComponentConfig::m_scattering, "Scattering", "Scattering coefficient.")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 10.0f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &VolumetricLightComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(Edit::UIHandlers::Slider, &VolumetricLightComponentConfig::m_extinction, "Extinction", "Absorption + out-scattering.")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 10.0f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &VolumetricLightComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(Edit::UIHandlers::Slider, &VolumetricLightComponentConfig::m_anisotropy, "Anisotropy", "Henyey-Greenstein phase function g. -1 back, +1 forward.")
                        ->Attribute(Edit::Attributes::Min, -1.0f)
                        ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &VolumetricLightComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(Edit::UIHandlers::Slider, &VolumetricLightComponentConfig::m_steps, "Raymarch Steps", "Number of samples along the ray.")
                        ->Attribute(Edit::Attributes::Min, 4.0f)
                        ->Attribute(Edit::Attributes::Max, 128.0f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &VolumetricLightComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(Edit::UIHandlers::Slider, &VolumetricLightComponentConfig::m_maxDistance, "Max Distance", "Far limit of the raymarch.")
                        ->Attribute(Edit::Attributes::Min, 1.0f)
                        ->Attribute(Edit::Attributes::Max, 1000.0f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &VolumetricLightComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(Edit::UIHandlers::Slider, &VolumetricLightComponentConfig::m_startDistance, "Start Distance", "Near limit of the raymarch.")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 100.0f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &VolumetricLightComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(Edit::UIHandlers::CheckBox, &VolumetricLightComponentConfig::m_enableShadows, "Enable Shadows", "Use directional cascade shadows for occlusion.")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &VolumetricLightComponentConfig::ArePropertiesReadOnly)

                        ->ClassElement(AZ::Edit::ClassElements::Group, "Overrides")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

#define EDITOR_CLASS VolumetricLightComponentConfig
#include <Atom/Feature/ParamMacros/StartOverrideEditorContext.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorVolumetricLightComponent>()->RequestBus("VolumetricLightRequestBus");

                behaviorContext->ConstantProperty("EditorVolumetricLightComponentTypeId", BehaviorConstant(Uuid(EditorVolumetricLightComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorVolumetricLightComponent::EditorVolumetricLightComponent(const VolumetricLightComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorVolumetricLightComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
