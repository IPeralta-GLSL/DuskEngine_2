/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/Feature/RayTracing/RayTracingFeatureProcessorInterface.h>
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <Render/DiffuseProbeGridRenderPass.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<Render::DiffuseProbeGridRenderPass> DiffuseProbeGridRenderPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew DiffuseProbeGridRenderPass(descriptor);
        }

        DiffuseProbeGridRenderPass::DiffuseProbeGridRenderPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
            if (!AZ_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED)
            {
                // GI is not supported on this platform
                SetEnabled(false);
                return;
            }

            // create the shader resource group
            // Note: the shader may not be available on all platforms
            AZStd::string shaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridRender.azshader";
            m_shader = RPI::LoadCriticalShader(shaderFilePath);
            if (m_shader == nullptr)
            {
                return;
            }

            m_srgLayout = m_shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Pass);
            AZ_Assert(m_srgLayout != nullptr, "[DiffuseProbeGridRenderPass '%s']: Failed to find SRG layout", GetPathName().GetCStr());

            m_shaderResourceGroup = RPI::ShaderResourceGroup::Create(m_shader->GetAsset(), m_shader->GetSupervariantIndex(), m_srgLayout->GetName());
            AZ_Assert(m_shaderResourceGroup, "[DiffuseProbeGridRenderPass '%s']: Failed to create SRG", GetPathName().GetCStr());
        }

        bool DiffuseProbeGridRenderPass::IsEnabled() const
        {
            if (!RenderPass::IsEnabled())
            {
                return false;
            }

            RPI::Scene* scene = m_pipeline->GetScene();
            if (!scene)
            {
                return false;
            }

            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            if (!diffuseProbeGridFeatureProcessor || diffuseProbeGridFeatureProcessor->GetProbeGrids().empty())
            {
                m_noGridFrames++;
                if (m_noGridFrames == 1 || (m_noGridFrames % 120) == 0)
                {
                    AZ_TracePrintf("DiffuseProbeGrid",
                        "[DiffuseProbeGridRenderPass] IsEnabled=false - no probe grids registered "
                        "(frame %u) hasFP=%d\n",
                        m_noGridFrames, diffuseProbeGridFeatureProcessor != nullptr);
                }
                // Scene emptied - reset so the next registration triggers a fresh TLAS wait
                m_wasEnabled = false;
                m_tlasWaitFrames = 0;
                m_gridRegisteredLogged = false;
                m_lastFirstProbeGrid = nullptr;
                return false;
            }
            m_noGridFrames = 0;

            // Detect probe grid set change (editor->game mode and back).
            // When the first real-time grid pointer changes the scene was rebuilt
            // and the TLAS wait must restart from scratch.
            const auto& realTimeGrids = diffuseProbeGridFeatureProcessor->GetRealTimeProbeGrids();
            const DiffuseProbeGrid* currentFirstGrid = realTimeGrids.empty() ? nullptr : realTimeGrids.front().get();
            if (currentFirstGrid != m_lastFirstProbeGrid)
            {
                AZ_TracePrintf("DiffuseProbeGrid",
                    "[DiffuseProbeGridRenderPass] Probe grid set changed (old=%p new=%p) - resetting TLAS wait\n",
                    m_lastFirstProbeGrid, currentFirstGrid);
                m_lastFirstProbeGrid   = currentFirstGrid;
                m_wasEnabled           = false;
                m_tlasWaitFrames       = 0;
                m_gridRegisteredLogged = false;
            }

            if (!m_gridRegisteredLogged)
            {
                m_gridRegisteredLogged = true;
                AZ_TracePrintf("DiffuseProbeGrid",
                    "[DiffuseProbeGridRenderPass] Probe grids registered: total=%zu realtime=%zu\n",
                    diffuseProbeGridFeatureProcessor->GetProbeGrids().size(),
                    realTimeGrids.size());
            }

            // For real-time probe grids, verify that the TLAS is ready and has geometry.
            if (!realTimeGrids.empty())
            {
                auto* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessorInterface>();

                const bool hasFP            = rayTracingFeatureProcessor != nullptr;
                const bool hasTlas          = hasFP && rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer() != nullptr;
                const uint32_t subMeshCount = hasFP ? rayTracingFeatureProcessor->GetSubMeshCount() : 0;
                const uint32_t pendingCount = hasFP ? rayTracingFeatureProcessor->GetPendingBlasCount() : 0;

                // Block only during an initial large BLAS build (>5% of submeshes pending).
                // Small incremental BLAS compaction cycles must NOT block or the atlas never converges.
                // Also allow through if pendingCount is <= 10 absolute (always safe to render).
                const bool initialBuildInProgress =
                    (subMeshCount == 0) ||
                    (pendingCount > 10 && pendingCount * 20 > subMeshCount);

                if (!hasFP || !hasTlas || initialBuildInProgress)
                {
                    m_tlasWaitFrames++;
                    m_wasEnabled = false;

                    // Log every frame for first 5 frames, then every 60 frames
                    if (m_tlasWaitFrames <= 5 || (m_tlasWaitFrames % 60) == 0)
                    {
                        AZ_TracePrintf("DiffuseProbeGrid",
                            "[DiffuseProbeGridRenderPass] IsEnabled=false - waiting for TLAS "
                            "(frame %u) | hasFP=%d | hasTlas=%d | subMeshCount=%u | pendingCount=%u | initialBuild=%d\n",
                            m_tlasWaitFrames, hasFP, hasTlas, subMeshCount, pendingCount, (int)initialBuildInProgress);
                    }
                    return false;
                }

                if (!m_wasEnabled)
                {
                    m_wasEnabled = true;
                    AZ_TracePrintf("DiffuseProbeGrid",
                        "[DiffuseProbeGridRenderPass] TLAS ready - probe rendering ENABLED "
                        "after %u frame(s) | subMeshCount=%u | pendingCount=%u\n",
                        m_tlasWaitFrames, subMeshCount, pendingCount);
                    m_tlasWaitFrames = 0;
                }
            }

            return true;
        }
        void DiffuseProbeGridRenderPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // get output attachment size
            AZ_Assert(GetInputOutputCount() > 0, "DiffuseProbeGridRenderPass: Could not find output bindings");
            RPI::PassAttachment* m_outputAttachment = GetInputOutputBinding(0).GetAttachment().get();
            AZ_Assert(m_outputAttachment, "DiffuseProbeGridRenderPass: Output binding has no attachment!");
            
            RHI::Size size = m_outputAttachment->m_descriptor.m_image.m_size;
            RHI::Viewport viewport(0.f, aznumeric_cast<float>(size.m_width), 0.f, aznumeric_cast<float>(size.m_height));
            RHI::Scissor scissor(0, 0, size.m_width, size.m_height);
            
            params.m_viewportState = viewport;
            params.m_scissorState = scissor;

            Base::FrameBeginInternal(params);

            // process attachment readback for RealTime grids, if raytracing is supported on this device
            if (RHI::RHISystemInterface::Get()->GetRayTracingSupport() != RHI::MultiDevice::NoDevices)
            {
                for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetRealTimeProbeGrids())
                {
                    diffuseProbeGrid->GetTextureReadback().FrameBegin(params);
                }
            }
        }

        void DiffuseProbeGridRenderPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGrids())
            {
                if (!ShouldRender(diffuseProbeGrid))
                {
                    continue;
                }

                // grid data
                {
                    RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetGridDataBufferAttachmentId();
                    desc.m_bufferViewDescriptor = diffuseProbeGrid->GetRenderData()->m_gridDataBufferViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::FragmentShader);
                }

                // probe irradiance image
                {
                    RHI::AttachmentId attachmentId = diffuseProbeGrid->GetIrradianceImageAttachmentId();
                    if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::Baked && !frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId))
                    {
                        // import the irradiance image now, since it is baked and therefore was not imported during the raytracing pass
                        [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(attachmentId, diffuseProbeGrid->GetIrradianceImage());
                        AZ_Assert(result == RHI::ResultCode::Success, "Failed to import probeIrradianceImage");
                    }

                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = attachmentId;
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeIrradianceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::FragmentShader);
                }

                // probe distance image
                {
                    RHI::AttachmentId attachmentId = diffuseProbeGrid->GetDistanceImageAttachmentId();
                    if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::Baked && !frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId))
                    {
                        // import the distance image now, since it is baked and therefore was not imported during the raytracing pass
                        [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(attachmentId, diffuseProbeGrid->GetDistanceImage());
                        AZ_Assert(result == RHI::ResultCode::Success, "Failed to import probeDistanceImage");
                    }

                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = attachmentId;
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDistanceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::FragmentShader);
                }

                // probe data image
                {
                    RHI::AttachmentId attachmentId = diffuseProbeGrid->GetProbeDataImageAttachmentId();
                    if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::Baked && !frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId))
                    {
                        // import the probe data image now, since it is baked and therefore was not imported during the raytracing pass
                        [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(attachmentId, diffuseProbeGrid->GetProbeDataImage());
                        AZ_Assert(result == RHI::ResultCode::Success, "Failed to import ProbeDataImage");
                    }

                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = attachmentId;
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDataImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;
                
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::FragmentShader);
                }

                diffuseProbeGrid->GetTextureReadback().Update(GetName());
            }

            Base::SetupFrameGraphDependencies(frameGraph);
        }

        void DiffuseProbeGridRenderPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGrids())
            {
                if (!ShouldRender(diffuseProbeGrid))
                {
                    continue;
                }

                // the diffuse probe grid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader inputs
                // (see ValidateSetImageView() of ShaderResourceGroupData.cpp)
                diffuseProbeGrid->UpdateRenderObjectSrg();

                if (!diffuseProbeGrid->GetRenderObjectSrg()->IsQueuedForCompile())
                {
                    diffuseProbeGrid->GetRenderObjectSrg()->Compile();
                }
            }

            if (auto viewSRG = diffuseProbeGridFeatureProcessor->GetViewSrg(m_pipeline, GetPipelineViewTag()))
            {
                BindSrg(viewSRG->GetRHIShaderResourceGroup());
            }

            Base::CompileResources(context);
        }

        bool DiffuseProbeGridRenderPass::ShouldRender(const AZStd::shared_ptr<DiffuseProbeGrid>& diffuseProbeGrid)
        {
            // check for baked mode with no valid textures
            if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::Baked &&
                !diffuseProbeGrid->HasValidBakedTextures())
            {
                return false;
            }

            // check for RealTime mode without ray tracing support
            if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::RealTime &&
                (RHI::RHISystemInterface::Get()->GetRayTracingSupport() == RHI::MultiDevice::NoDevices))
            {
                return false;
            }

            // check for RealTime mode with ray tracing support but no valid TLAS
            // Block only during initial large BLAS build (>5% pending), not during
            // incremental BLAS compaction cycles which only affect a few BLASes.
            if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::RealTime)
            {
                RPI::Scene* scene = m_pipeline->GetScene();
                auto* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessorInterface>();
                if (rayTracingFeatureProcessor)
                {
                    const bool hasTlas          = rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer() != nullptr;
                    const uint32_t subMeshCount = rayTracingFeatureProcessor->GetSubMeshCount();
                    const uint32_t pendingCount = rayTracingFeatureProcessor->GetPendingBlasCount();
                    const bool initialBuildInProgress = (subMeshCount == 0) || (pendingCount * 20 > subMeshCount);

                    if (!hasTlas || initialBuildInProgress)
                    {
                        m_shouldRenderSkipCount++;
                        if (m_shouldRenderSkipCount == 1 || (m_shouldRenderSkipCount % 60) == 0)
                        {
                            AZ_TracePrintf("DiffuseProbeGrid",
                                "[DiffuseProbeGridRenderPass::ShouldRender] Skipping grid @%p (skip#%u) "
                                "(hasTlas=%d subMeshCount=%u pendingCount=%u)\n",
                                diffuseProbeGrid.get(),
                                m_shouldRenderSkipCount,
                                hasTlas,
                                subMeshCount,
                                pendingCount);
                        }
                        return false;
                    }
                }
            }

            // check if culled out
            if (!diffuseProbeGrid->GetIsVisible())
            {
                return false;
            }

            // DiffuseProbeGrid should be rendered
            return true;
        }
    } // namespace Render
} // namespace AZ
