/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace Render
    {
        struct LightTreeGpuNode
        {
            float m_bboxMin[3];
            float m_bboxMax[3];
            float m_coneAxis[3];
            float m_coneThetaO;
            float m_energy;
            uint32_t m_leftChildIndex;
            uint32_t m_rightChildIndex;
            uint32_t m_lightIndex;
            uint32_t m_lightCount;
        };

        struct LightTreeInput
        {
            Vector3 m_position;
            Vector3 m_direction;
            float m_energy;
            float m_radius;
            uint32_t m_lightIndex;
        };

        class LightTreeBuilder
        {
        public:
            AZ_CLASS_ALLOCATOR(LightTreeBuilder, SystemAllocator);

            void Build(const AZStd::vector<LightTreeInput>& lights);

            const AZStd::vector<LightTreeGpuNode>& GetNodes() const { return m_nodes; }
            const AZStd::vector<uint32_t>& GetLightIndices() const { return m_lightIndices; }
            uint32_t GetNodeCount() const { return static_cast<uint32_t>(m_nodes.size()); }

        private:
            struct Bucket
            {
                Aabb m_bbox = Aabb::CreateNull();
                float m_energy = 0.0f;
                uint32_t m_count = 0;
                float m_coneAxis[3] = { 0.0f, 0.0f, 0.0f };
                float m_coneThetaO = 1.0f;
            };

            struct SplitCandidate
            {
                uint32_t m_axis = 0;
                float m_cost = FLT_MAX;
                uint32_t m_bucketIndex = 0;
            };

            static constexpr uint32_t NumBuckets = 12;
            static constexpr uint32_t MaxLightsPerLeaf = 4;
            static constexpr float TraversalCost = 1.0f;

            uint32_t BuildRecursive(AZStd::span<uint32_t> indices);
            float FindBestSplit(AZStd::span<uint32_t> indices, uint32_t& bestAxis, uint32_t& bestBucket);
            float EvaluateSplit(const Bucket* leftBuckets, uint32_t leftCount,
                               const Bucket* rightBuckets, uint32_t rightCount,
                               const Aabb& parentBbox);

            void ComputeNodeCone(const AZStd::vector<LightTreeInput>& lights,
                                AZStd::span<uint32_t> indices,
                                float outAxis[3], float& outThetaO);

            AZStd::vector<LightTreeGpuNode> m_nodes;
            AZStd::vector<uint32_t> m_lightIndices;
            AZStd::vector<LightTreeInput> m_lights;
        };
    }
}
