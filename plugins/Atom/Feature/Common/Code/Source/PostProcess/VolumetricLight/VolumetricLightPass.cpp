/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/VolumetricLight/VolumetricLightPass.h>

#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        AZ_CVAR(bool, r_volumetricLightEnabled, true, nullptr, AZ::ConsoleFunctorFlags::Null, "Enable volumetric light");

        VolumetricLightPass::VolumetricLightPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
            , m_currentTime(0.0f)
        {
        }

        RPI::Ptr<VolumetricLightPass> VolumetricLightPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<VolumetricLightPass> pass = aznew VolumetricLightPass(descriptor);
            pass->SetSrgBindIndices();
            VolumetricLightSettings* settings = pass->GetPassSettings();
            bool isEnabled = pass->Pass::IsEnabled();
            settings->SetEnabled(isEnabled);
            return AZStd::move(pass);
        }

        void VolumetricLightPass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();
        }

        VolumetricLightSettings* VolumetricLightPass::GetPassSettings()
        {
            RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return &m_fallbackSettings;
            }

            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = m_pipeline->GetFirstView(GetPipelineViewTag());
            if (fp)
            {
                PostProcessSettings* ppSettings = fp->GetLevelSettingsFromView(view);
                if (ppSettings)
                {
                    VolumetricLightSettings* settings = ppSettings->GetVolumetricLightSettings();
                    if (settings)
                    {
                        m_fallbackSettings.SetEnabled(false);
                    }
                    return settings ? settings : &m_fallbackSettings;
                }
            }
            return &m_fallbackSettings;
        }

        void VolumetricLightPass::SetSrgBindIndices()
        {
            VolumetricLightSettings* settings = GetPassSettings();
            Data::Instance<RPI::ShaderResourceGroup> srg = m_shaderResourceGroup.get();

#define AZ_GFX_COMMON_PARAM(ValueType, ParamName, MemberName, DefaultValue)                                            settings->MemberName##SrgIndex = srg->FindShaderInputConstantIndex(AZ::Name(#MemberName));      
#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            settings->SetInitialized(true);
            m_depthTextureDimensionsIndex = srg->FindShaderInputConstantIndex(Name("m_depthTextureDimensions"));
            m_noiseTextureIndex = srg->FindShaderInputImageIndex(Name("m_noiseTexture"));
        }

        void VolumetricLightPass::SetSrgConstants()
        {
            VolumetricLightSettings* settings = GetPassSettings();
            Data::Instance<RPI::ShaderResourceGroup> srg = m_shaderResourceGroup.get();

            if (!settings->IsInitialized())
            {
                SetSrgBindIndices();
            }

            if (settings->GetSettingsNeedUpdate())
            {
                settings->SetSettingsNeedUpdate(false);
            }

#define AZ_GFX_COMMON_PARAM(ValueType, ParamName, MemberName, DefaultValue)                                  \
            if (settings->MemberName##SrgIndex.IsValid())                                               \
            {                                                                                           \
                srg->SetConstant(settings->MemberName##SrgIndex, settings->MemberName);                 \
            }                                                                                           \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            if (m_depthTextureDimensionsIndex.IsValid())
            {
                auto attachment = GetInputOutputBinding(0).GetAttachment();
                if (attachment)
                {
                    const auto& descriptor = attachment->GetTransientImageDescriptor().m_imageDescriptor;
                    float dims[] = { static_cast<float>(descriptor.m_size.m_width), static_cast<float>(descriptor.m_size.m_height) };
                    srg->SetConstant(m_depthTextureDimensionsIndex, dims);
                }
            }

            m_currentTime += 0.016f;
        }

        void VolumetricLightPass::UpdateEnable(VolumetricLightSettings* settings)
        {
            if (!m_pipeline || !settings)
            {
                SetEnabled(false);
                return;
            }
            if (IsEnabled() == settings->GetEnabled())
            {
                return;
            }
            SetEnabled(settings->GetEnabled());
        }

        bool VolumetricLightPass::IsEnabled() const
        {
            if (!r_volumetricLightEnabled)
            {
                return false;
            }
            const VolumetricLightSettings* settings = const_cast<VolumetricLightPass*>(this)->GetPassSettings();
            return settings->GetEnabled();
        }

        void VolumetricLightPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            FullscreenTrianglePass::SetupFrameGraphDependencies(frameGraph);
            VolumetricLightSettings* settings = GetPassSettings();
            UpdateEnable(settings);
            SetSrgConstants();
        }
    }
}
