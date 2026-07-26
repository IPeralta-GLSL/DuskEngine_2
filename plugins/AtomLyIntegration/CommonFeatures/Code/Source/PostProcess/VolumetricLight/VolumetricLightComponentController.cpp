/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/Scene.h>

#include <PostProcess/VolumetricLight/VolumetricLightComponentController.h>

namespace AZ
{
    namespace Render
    {
        void VolumetricLightComponentController::Reflect(ReflectContext* context)
        {
            VolumetricLightComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<VolumetricLightComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &VolumetricLightComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<VolumetricLightRequestBus>("VolumetricLightRequestBus")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)

#define PARAM_EVENT_BUS VolumetricLightRequestBus::Events
#include <Atom/Feature/ParamMacros/StartParamBehaviorContext.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef PARAM_EVENT_BUS

                    ;
            }
        }

        void VolumetricLightComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("VolumetricLightService"));
        }

        void VolumetricLightComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("VolumetricLightService"));
        }

        void VolumetricLightComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("PostFXLayerService"));
        }

        VolumetricLightComponentController::VolumetricLightComponentController(const VolumetricLightComponentConfig& config)
            : m_configuration(config)
        {
        }

        void VolumetricLightComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            PostProcessFeatureProcessorInterface* fp = RPI::Scene::GetFeatureProcessorForEntity<PostProcessFeatureProcessorInterface>(m_entityId);
            if (fp)
            {
                m_postProcessInterface = fp->GetOrCreateSettingsInterface(m_entityId);
                if (m_postProcessInterface)
                {
                    m_settingsInterface = m_postProcessInterface->GetOrCreateVolumetricLightSettingsInterface();
                    OnConfigChanged();
                }
            }
            VolumetricLightRequestBus::Handler::BusConnect(m_entityId);
        }

        void VolumetricLightComponentController::Deactivate()
        {
            VolumetricLightRequestBus::Handler::BusDisconnect(m_entityId);

            m_configuration.SetEnabled(false);
            OnConfigChanged();

            if (m_postProcessInterface)
            {
                m_postProcessInterface->RemoveVolumetricLightSettingsInterface();
            }

            m_postProcessInterface = nullptr;
            m_settingsInterface = nullptr;
            m_entityId.SetInvalid();
        }

        void VolumetricLightComponentController::SetConfiguration(const VolumetricLightComponentConfig& config)
        {
            m_configuration = config;
            OnConfigChanged();
        }

        const VolumetricLightComponentConfig& VolumetricLightComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void VolumetricLightComponentController::OnConfigChanged()
        {
            if (m_settingsInterface)
            {
                m_configuration.CopySettingsTo(m_settingsInterface);
                m_settingsInterface->OnSettingsChanged();
            }
        }

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType VolumetricLightComponentController::Get##Name() const                                   \
        {                                                                                               \
            return m_configuration.MemberName;                                                          \
        }                                                                                               \
        void VolumetricLightComponentController::Set##Name(ValueType val)                               \
        {                                                                                               \
            if(m_settingsInterface)                                                                     \
            {                                                                                           \
                m_settingsInterface->Set##Name(val);                                                    \
                m_settingsInterface->OnSettingsChanged();                                                 \
                m_configuration.MemberName = m_settingsInterface->Get##Name();                          \
            }                                                                                           \
            else                                                                                        \
            {                                                                                           \
                m_configuration.MemberName = val;                                                       \
            }                                                                                           \
        }                                                                                               \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        OverrideValueType VolumetricLightComponentController::Get##Name##Override() const               \
        {                                                                                               \
            return m_configuration.MemberName##Override;                                                \
        }                                                                                               \
        void VolumetricLightComponentController::Set##Name##Override(OverrideValueType val)             \
        {                                                                                               \
            m_configuration.MemberName##Override = val;                                                 \
            if(m_settingsInterface)                                                                     \
            {                                                                                           \
                m_settingsInterface->Set##Name##Override(val);                                          \
                m_settingsInterface->OnSettingsChanged();                                                 \
            }                                                                                           \
        }                                                                                               \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
    }
}
