/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/ReflectionProbe/ReflectionProbeFeatureProcessorInterface.h>
#include <ReflectionProbe/ReflectionProbe.h>
#include <ReflectionProbe/ReflectionProbeSSSR.h>

namespace AZ
{
    namespace Render
    {
        class ReflectionProbe;
        using ReflectionProbePtr = AZStd::shared_ptr<ReflectionProbe>;
        using ReflectionProbeVector = AZStd::vector<ReflectionProbePtr>;

        class ReflectionProbeFeatureProcessor final
            : public ReflectionProbeFeatureProcessorInterface,
              private Data::AssetBus::MultiHandler
        {
        public:
            AZ_CLASS_ALLOCATOR(ReflectionProbeFeatureProcessor, AZ::SystemAllocator)
            AZ_RTTI(AZ::Render::ReflectionProbeFeatureProcessor, "{A08C591F-D2AB-4550-852A-4436533DB137}", AZ::Render::ReflectionProbeFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            ReflectionProbeFeatureProcessor() = default;
            virtual ~ReflectionProbeFeatureProcessor() = default;

            // ReflectionProbeFeatureProcessorInterface overrides
            ReflectionProbeHandle AddReflectionProbe(const AZ::Transform& transform, bool useParallaxCorrection) override;
            void RemoveReflectionProbe(ReflectionProbeHandle& handle) override;
            bool IsValidHandle(const ReflectionProbeHandle& handle) const override { return (m_reflectionProbeMap.find(handle) != m_reflectionProbeMap.end()); }

            void SetOuterExtents(const ReflectionProbeHandle& handle, const AZ::Vector3& outerExtents) override;
            AZ::Vector3 GetOuterExtents(const ReflectionProbeHandle& handle) const override;

            void SetInnerExtents(const ReflectionProbeHandle& handle, const AZ::Vector3& innerExtents) override;
            AZ::Vector3 GetInnerExtents(const ReflectionProbeHandle& handle) const override;

            AZ::Obb GetOuterObbWs(const ReflectionProbeHandle& handle) const override;
            AZ::Obb GetInnerObbWs(const ReflectionProbeHandle& handle) const override;

            void SetTransform(const ReflectionProbeHandle& handle, const AZ::Transform& transform) override;
            AZ::Transform GetTransform(const ReflectionProbeHandle& handle) const override;

            void SetCubeMap(const ReflectionProbeHandle& handle, Data::Instance<RPI::Image>& cubeMapImage, const AZStd::string& relativePath) override;
            Data::Instance<RPI::Image> GetCubeMap(const ReflectionProbeHandle& handle) const override;

            void SetRenderExposure(const ReflectionProbeHandle& handle, float renderExposure) override;
            float GetRenderExposure(const ReflectionProbeHandle& handle) const override;

            void SetBakeExposure(const ReflectionProbeHandle& handle, float bakeExposure) override;
            float GetBakeExposure(const ReflectionProbeHandle& handle) const override;

            bool GetUseParallaxCorrection(const ReflectionProbeHandle& handle) const override;

            void Bake(const ReflectionProbeHandle& handle, BuildCubeMapCallback callback, const AZStd::string& relativePath) override;
            bool CheckCubeMapAssetNotification(const AZStd::string& relativePath, Data::Asset<RPI::StreamingImageAsset>& outCubeMapAsset, CubeMapAssetNotificationType& outNotificationType) override;
            bool IsCubeMapReferenced(const AZStd::string& relativePath) override;
            void ShowVisualization(const ReflectionProbeHandle& handle, bool showVisualization) override;
            void FindReflectionProbes(const AZ::Vector3& position, ReflectionProbeHandleVector& reflectionProbeHandles) override;
            void FindReflectionProbes(const AZ::Aabb& aabb, ReflectionProbeHandleVector& reflectionProbeHandles) override;

            void SetMode(const ReflectionProbeHandle& handle, ReflectionProbeMode mode) override;
            ReflectionProbeMode GetMode(const ReflectionProbeHandle& handle) const override;

            ReflectionProbeVector& GetReflectionProbes() { return m_reflectionProbes; }
            ReflectionProbeVector& GetRealTimeReflectionProbes() { return m_realTimeReflectionProbes; }
            ReflectionProbeVector& GetVisibleReflectionProbes() { return m_visibleReflectionProbes; }
            ReflectionProbeVector& GetVisibleRealTimeReflectionProbes() { return m_visibleRealTimeReflectionProbes; }

            // FeatureProcessor overrides
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
            void OnRenderEnd() override;

        private:

            AZ_DISABLE_COPY_MOVE(ReflectionProbeFeatureProcessor);

            void CreateBoxMesh();

            void LoadShader(
                const char* filePath, RPI::Ptr<RPI::PipelineStateForDraw>& pipelineState, Data::Instance<RPI::Shader>& shader,
                RHI::Ptr<RHI::ShaderResourceGroupLayout>& srgLayout, RHI::DrawListTag& drawListTag);

            void OnRenderPipelineChanged(AZ::RPI::RenderPipeline* pipeline, RPI::SceneNotification::RenderPipelineChangeType changeType) override;

            void UpdatePipelineStates();

            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetError(Data::Asset<Data::AssetData> asset) override;

            void HandleAssetNotification(Data::Asset<Data::AssetData> asset, CubeMapAssetNotificationType notificationType);

            void FindReflectionProbesInternal(const AZ::Aabb& aabb, ReflectionProbeHandleVector& reflectionProbes, AZStd::function<bool(const ReflectionProbe*)> filter = {});

            void UpdateRealTimeList(const ReflectionProbePtr& reflectionProbe);

            bool ValidateHandle(const ReflectionProbeHandle& handle) const;

            using ReflectionProbeMap = AZStd::unordered_map <ReflectionProbeHandle, ReflectionProbePtr>;
            ReflectionProbeMap m_reflectionProbeMap;

            const size_t InitialProbeAllocationSize = 64;
            ReflectionProbeVector m_reflectionProbes;
            ReflectionProbeVector m_realTimeReflectionProbes;
            ReflectionProbeVector m_visibleReflectionProbes;
            ReflectionProbeVector m_visibleRealTimeReflectionProbes;

            struct NotifyCubeMapAssetEntry
            {
                AZStd::string m_relativePath;
                AZ::Data::AssetId m_assetId;
                Data::Asset<RPI::StreamingImageAsset> m_asset;
                CubeMapAssetNotificationType m_notificationType = CubeMapAssetNotificationType::None;
            };
            typedef AZStd::vector<NotifyCubeMapAssetEntry> NotifyCubeMapAssetVector;
            NotifyCubeMapAssetVector m_notifyCubeMapAssets;

            struct Position
            {
                float m_x = 0.0f;
                float m_y = 0.0f;
                float m_z = 0.0f;
            };

            RHI::Ptr<RHI::BufferPool> m_bufferPool;

            AZStd::vector<Position> m_boxPositions;
            AZStd::vector<uint16_t> m_boxIndices;
            RHI::Ptr<RHI::Buffer> m_boxPositionBuffer;
            RHI::Ptr<RHI::Buffer> m_boxIndexBuffer;
            RHI::InputStreamLayout m_boxStreamLayout;

            ReflectionRenderData m_reflectionRenderData;

            // SSSR (Stochastic Screen Space Reflections) support
            AZStd::unique_ptr<ReflectionProbeSSSR> m_sssr;

            bool m_probeSortRequired = false;
            bool m_meshFeatureProcessorUpdateRequired = false;
            bool m_needUpdatePipelineStates = false;
        };
    } // namespace Render
} // namespace AZ