/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/Ssao/SsaoSettings.h>
#include <PostProcessing/SsaoPasses.h>
#include <AzCore/Math/MathUtils.h>
#include <Atom/Feature/PostProcess/Ssao/SsaoConstants.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <cmath>

namespace AZ
{
    namespace Render
    {
        // --- SSAO Parent Pass ---

        RPI::Ptr<SsaoParentPass> SsaoParentPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<SsaoParentPass> pass = aznew SsaoParentPass(descriptor);
            return AZStd::move(pass);
        }

        SsaoParentPass::SsaoParentPass(const RPI::PassDescriptor& descriptor)
            : RPI::ParentPass(descriptor)
        { }

        bool SsaoParentPass::IsEnabled() const
        {
            if (!ParentPass::IsEnabled())
            {
                return false;
            }
            const RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return false;
            }
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            const RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
            if (!fp)
            {
                return true;
            }
            PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
            if (!postProcessSettings)
            {
                return true;
            }
            const SsaoSettings* ssaoSettings = postProcessSettings->GetSsaoSettings();
            if (!ssaoSettings)
            {
                return true;
            }
            return ssaoSettings->GetEnabled();
        }

        void SsaoParentPass::InitializeInternal()
        {
            ParentPass::InitializeInternal();
        }

        void SsaoParentPass::FrameBeginInternal(FramePrepareParams params)
        {
            ParentPass::FrameBeginInternal(params);
        }

        // --- SSAO Compute Pass ---

        RPI::Ptr<SsaoComputePass> SsaoComputePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<SsaoComputePass> pass = aznew SsaoComputePass(descriptor);
            return AZStd::move(pass);
        }

        SsaoComputePass::SsaoComputePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        { }

        void SsaoComputePass::FrameBeginInternal(FramePrepareParams params)
        {
            // Must match CacaoConstants in CacaoGenerateQ2.azsl / CacaoEdgeSensitiveBlur.azsl
            struct alignas(16) CacaoConstants
            {
                // NDC-UV → view-space:  viewXY = (uv * Mul + Add) * linearDepth
                float m_ndcToViewMulX, m_ndcToViewMulY;
                float m_ndcToViewAddX, m_ndcToViewAddY;

                // Inverse SSAO-buffer (quarter-res) and full-res dimensions
                float m_ssaoBufInvW, m_ssaoBufInvH;
                float m_outputBufInvW, m_outputBufInvH;

                float m_effectRadius;
                float m_effectShadowStrength;
                float m_effectShadowPow;
                float m_effectShadowClamp;

                float m_effectFadeOutMul;
                float m_effectFadeOutAdd;
                float m_effectHorizonAngleThreshold;
                float m_effectSamplingRadiusNearLimitRec;

                float m_negRecEffectRadius;
                float m_invSharpness;
                float m_bilateralSigmaSquared;
                float m_bilateralSimilarityDistanceSigma;

                float m_detailAOStrength;
                float m_depthPrecisionOffsetMod;
                float m_pad0, m_pad1;

                // Per-layer, per-subpass rotation+scale matrices (4×5)
                float m_patternRotScaleMatrices[4][5][4];
            } c{};

            // --- Retrieve output attachment dimensions (SSAO buffer = W/2 × H/2) ---
            AZ_Assert(GetOutputCount() > 0, "SsaoComputePass (CACAO): No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();
            AZ_Assert(outputAttachment != nullptr, "SsaoComputePass (CACAO): Output binding has no attachment!");
            const RHI::Size size = outputAttachment->m_descriptor.m_image.m_size;

            const float ssaoBufW = static_cast<float>(size.m_width);
            const float ssaoBufH = static_cast<float>(size.m_height);
            c.m_ssaoBufInvW    = 1.0f / ssaoBufW;
            c.m_ssaoBufInvH    = 1.0f / ssaoBufH;
            c.m_outputBufInvW  = 0.5f / ssaoBufW;   // full-res = ssao * 2
            c.m_outputBufInvH  = 0.5f / ssaoBufH;

            // --- View-dependent constants ---
            AZ::RPI::ViewPtr view = m_pipeline->GetFirstView(GetPipelineViewTag());
            float tanHalfFovX = 1.0f;
            float tanHalfFovY = 1.0f;
            if (view)
            {
                const AZ::Matrix4x4& ctv = view->GetClipToViewMatrix();
                tanHalfFovX = ctv.GetElement(0, 0);
                tanHalfFovY = ctv.GetElement(1, 1);

                c.m_ndcToViewMulX =  2.0f * tanHalfFovX;
                c.m_ndcToViewMulY = -2.0f * tanHalfFovY;
                c.m_ndcToViewAddX = -tanHalfFovX;
                c.m_ndcToViewAddY =  tanHalfFovY;
            }

            // --- CACAO default effect parameters ---
            const float effectRadius = 1.2f;
            const float fadeOutFrom  = 50.0f;
            const float fadeOutTo    = 300.0f;

            c.m_effectRadius                         = effectRadius;
            c.m_effectShadowStrength                 = 4.3f;
            c.m_effectShadowPow                      = 1.5f;
            c.m_effectShadowClamp                    = 0.98f;
            c.m_effectFadeOutMul                     = -1.0f / (fadeOutTo - fadeOutFrom);
            c.m_effectFadeOutAdd                     = fadeOutFrom / (fadeOutTo - fadeOutFrom) + 1.0f;
            c.m_effectHorizonAngleThreshold          = 0.06f;
            c.m_effectSamplingRadiusNearLimitRec     = tanHalfFovY / (effectRadius * 1.2f);
            c.m_negRecEffectRadius                   = -1.0f / effectRadius;
            c.m_depthPrecisionOffsetMod              = 0.9992f;
            c.m_detailAOStrength                     = 0.5f;
            c.m_invSharpness                         = 0.0f;
            c.m_bilateralSigmaSquared                = 5.0f;
            c.m_bilateralSimilarityDistanceSigma     = 0.1f;

            // --- Override from SsaoSettings if available ---
            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene ? scene->GetFeatureProcessor<PostProcessFeatureProcessor>() : nullptr;
            if (fp && view)
            {
                PostProcessSettings* pps = fp->GetLevelSettingsFromView(view);
                if (pps)
                {
                    SsaoSettings* s = pps->GetSsaoSettings();
                    if (s)
                    {
                        if (s->GetEnabled())
                        {
                            c.m_effectShadowStrength             = s->GetStrength() * 4.3f;
                            c.m_bilateralSigmaSquared            = s->GetBlurConstFalloff() * 10.0f;
                            c.m_bilateralSimilarityDistanceSigma = 1.0f / (s->GetBlurDepthFalloffStrength() + 1.0f);
                            c.m_invSharpness                     = AZ::GetMax(0.0f, 1.0f - s->GetBlurConstFalloff());
                        }
                        else
                        {
                            c.m_effectShadowStrength = 0.0f;
                        }
                    }
                }
            }

            // --- PatternRotScaleMatrices: CACAO's sub-pass sample distribution ---
            // angle = (pass + spmap[sub]/5) * PI/2
            // scale = 1 + (pass - 1.5 + (spmap[sub] - 2) / 5) * 0.07
            // matrix = { scale*cos, -scale*sin, scale*sin, -scale*cos }
            static const int spmap[5] = { 0, 1, 4, 3, 2 };
            const float piHalf = AZ::Constants::HalfPi;
            for (int pass = 0; pass < 4; ++pass)
            {
                for (int sub = 0; sub < 5; ++sub)
                {
                    const float angle = (static_cast<float>(pass) +
                                         static_cast<float>(spmap[sub]) / 5.0f) * piHalf;
                    const float scale = 1.0f + (static_cast<float>(pass) - 1.5f +
                                                (static_cast<float>(spmap[sub]) - 2.0f) / 5.0f) * 0.07f;
                    float* m = c.m_patternRotScaleMatrices[pass][sub];
                    m[0] =  scale * cosf(angle);
                    m[1] = -scale * sinf(angle);
                    m[2] =  scale * sinf(angle);
                    m[3] = -scale * cosf(angle);
                }
            }

            if (m_shaderResourceGroup)
            {
                m_shaderResourceGroup->SetConstant(m_constantsIndex, c);
            }

            RPI::ComputePass::FrameBeginInternal(params);
        }

    }   // namespace Render
}   // namespace AZ
