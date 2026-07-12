/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LightTreeBuilder.h"

#include <AzCore/Math/MathUtils.h>
#include <algorithm>
#include <cmath>

namespace AZ
{
    namespace Render
    {
        void LightTreeBuilder::Build(const AZStd::vector<LightTreeInput>& lights)
        {
            m_nodes.clear();
            m_lightIndices.clear();
            m_lights = lights;

            if (m_lights.empty())
            {
                return;
            }

            m_lightIndices.resize(m_lights.size());
            for (uint32_t i = 0; i < static_cast<uint32_t>(m_lights.size()); ++i)
            {
                m_lightIndices[i] = i;
            }

            BuildRecursive(AZStd::span<uint32_t>(m_lightIndices.data(), m_lightIndices.size()));
        }

        uint32_t LightTreeBuilder::BuildRecursive(AZStd::span<uint32_t> indices)
        {
            uint32_t nodeIndex = static_cast<uint32_t>(m_nodes.size());
            m_nodes.push_back(LightTreeGpuNode{});

            LightTreeGpuNode& node = m_nodes[nodeIndex];

            Aabb bbox = Aabb::CreateNull();
            float totalEnergy = 0.0f;
            for (uint32_t idx : indices)
            {
                const LightTreeInput& light = m_lights[idx];
                Aabb lightAabb = Aabb::CreateCenterRadius(light.m_position, light.m_radius);
                bbox.AddAabb(lightAabb);
                totalEnergy += light.m_energy;
            }

            node.m_bboxMin[0] = bbox.GetMin().GetX();
            node.m_bboxMin[1] = bbox.GetMin().GetY();
            node.m_bboxMin[2] = bbox.GetMin().GetZ();
            node.m_bboxMax[0] = bbox.GetMax().GetX();
            node.m_bboxMax[1] = bbox.GetMax().GetY();
            node.m_bboxMax[2] = bbox.GetMax().GetZ();
            node.m_energy = totalEnergy;

            float coneAxis[3];
            float coneThetaO;
            ComputeNodeCone(m_lights, indices, coneAxis, coneThetaO);
            node.m_coneAxis[0] = coneAxis[0];
            node.m_coneAxis[1] = coneAxis[1];
            node.m_coneAxis[2] = coneAxis[2];
            node.m_coneThetaO = coneThetaO;

            if (indices.size() <= MaxLightsPerLeaf)
            {
                node.m_lightIndex = static_cast<uint32_t>(m_lightIndices.size());
                node.m_lightCount = static_cast<uint32_t>(indices.size());

                for (uint32_t idx : indices)
                {
                    m_lightIndices.push_back(m_lights[idx].m_lightIndex);
                }

                node.m_leftChildIndex = 0;
                node.m_rightChildIndex = 0;
                return nodeIndex;
            }

            uint32_t bestAxis = 0;
            uint32_t bestBucket = 0;
            float splitCost = FindBestSplit(indices, bestAxis, bestBucket);

            float leafCost = totalEnergy * bbox.GetSurfaceArea();

            if (splitCost >= leafCost || indices.size() <= MaxLightsPerLeaf)
            {
                node.m_lightIndex = static_cast<uint32_t>(m_lightIndices.size());
                node.m_lightCount = static_cast<uint32_t>(indices.size());

                for (uint32_t idx : indices)
                {
                    m_lightIndices.push_back(m_lights[idx].m_lightIndex);
                }

                node.m_leftChildIndex = 0;
                node.m_rightChildIndex = 0;
                return nodeIndex;
            }

            Vector3 extent = bbox.GetMax() - bbox.GetMin();
            float minBound = 0.0f;
            float range = 1.0f;

            switch (bestAxis)
            {
            case 0:
                minBound = bbox.GetMin().GetX();
                range = extent.GetX();
                break;
            case 1:
                minBound = bbox.GetMin().GetY();
                range = extent.GetY();
                break;
            case 2:
                minBound = bbox.GetMin().GetZ();
                range = extent.GetZ();
                break;
            }

            float splitPos = minBound + (static_cast<float>(bestBucket) + 1.0f) / static_cast<float>(NumBuckets) * range;

            AZStd::vector<uint32_t> leftIndices, rightIndices;
            leftIndices.reserve(indices.size());
            rightIndices.reserve(indices.size());

            for (uint32_t idx : indices)
            {
                const LightTreeInput& light = m_lights[idx];
                float pos = 0.0f;
                switch (bestAxis)
                {
                case 0: pos = light.m_position.GetX(); break;
                case 1: pos = light.m_position.GetY(); break;
                case 2: pos = light.m_position.GetZ(); break;
                }

                if (pos < splitPos)
                {
                    leftIndices.push_back(idx);
                }
                else
                {
                    rightIndices.push_back(idx);
                }
            }

            if (leftIndices.empty() || rightIndices.empty())
            {
                uint32_t mid = static_cast<uint32_t>(indices.size()) / 2;
                leftIndices.clear();
                rightIndices.clear();
                for (uint32_t i = 0; i < indices.size(); ++i)
                {
                    if (i < mid)
                        leftIndices.push_back(indices[i]);
                    else
                        rightIndices.push_back(indices[i]);
                }
            }

            uint32_t leftChild = BuildRecursive(AZStd::span<uint32_t>(leftIndices.data(), leftIndices.size()));
            node = m_nodes[nodeIndex];
            uint32_t rightChild = BuildRecursive(AZStd::span<uint32_t>(rightIndices.data(), rightIndices.size()));
            node = m_nodes[nodeIndex];

            node.m_leftChildIndex = leftChild;
            node.m_rightChildIndex = rightChild;
            node.m_lightIndex = 0;
            node.m_lightCount = 0;

            return nodeIndex;
        }

        float LightTreeBuilder::FindBestSplit(AZStd::span<uint32_t> indices, uint32_t& bestAxis, uint32_t& bestBucket)
        {
            Aabb bbox = Aabb::CreateNull();
            for (uint32_t idx : indices)
            {
                bbox.AddPoint(m_lights[idx].m_position);
            }

            float bestCost = FLT_MAX;

            for (uint32_t axis = 0; axis < 3; ++axis)
            {
                Bucket buckets[NumBuckets] = {};

                float minBound = 0.0f;
                float range = 1.0f;
                Vector3 extent = bbox.GetMax() - bbox.GetMin();

                switch (axis)
                {
                case 0:
                    minBound = bbox.GetMin().GetX();
                    range = extent.GetX();
                    break;
                case 1:
                    minBound = bbox.GetMin().GetY();
                    range = extent.GetY();
                    break;
                case 2:
                    minBound = bbox.GetMin().GetZ();
                    range = extent.GetZ();
                    break;
                }

                if (range < 1e-6f)
                    continue;

                for (uint32_t idx : indices)
                {
                    const LightTreeInput& light = m_lights[idx];
                    float pos = 0.0f;
                    switch (axis)
                    {
                    case 0: pos = light.m_position.GetX(); break;
                    case 1: pos = light.m_position.GetY(); break;
                    case 2: pos = light.m_position.GetZ(); break;
                    }

                    float normalized = (pos - minBound) / range;
                    uint32_t bucketIdx = static_cast<uint32_t>(normalized * NumBuckets);
                    bucketIdx = AZStd::min(bucketIdx, NumBuckets - 1);

                    Bucket& bucket = buckets[bucketIdx];
                    Aabb lightAabb = Aabb::CreateCenterRadius(light.m_position, light.m_radius);
                    bucket.m_bbox.AddAabb(lightAabb);
                    bucket.m_energy += light.m_energy;
                    bucket.m_count++;
                }

                Bucket leftBuckets[NumBuckets] = {};
                Bucket rightBuckets[NumBuckets] = {};

                leftBuckets[0] = buckets[0];
                for (uint32_t i = 1; i < NumBuckets; ++i)
                {
                    leftBuckets[i].m_bbox = leftBuckets[i - 1].m_bbox;
                    leftBuckets[i].m_bbox.AddAabb(buckets[i].m_bbox);
                    leftBuckets[i].m_energy = leftBuckets[i - 1].m_energy + buckets[i].m_energy;
                    leftBuckets[i].m_count = leftBuckets[i - 1].m_count + buckets[i].m_count;
                }

                rightBuckets[NumBuckets - 1] = buckets[NumBuckets - 1];
                for (int i = NumBuckets - 2; i >= 0; --i)
                {
                    rightBuckets[i].m_bbox = rightBuckets[i + 1].m_bbox;
                    rightBuckets[i].m_bbox.AddAabb(buckets[i].m_bbox);
                    rightBuckets[i].m_energy = rightBuckets[i + 1].m_energy + buckets[i].m_energy;
                    rightBuckets[i].m_count = rightBuckets[i + 1].m_count + buckets[i].m_count;
                }

                for (uint32_t i = 0; i < NumBuckets - 1; ++i)
                {
                    if (leftBuckets[i].m_count == 0 || rightBuckets[i + 1].m_count == 0)
                        continue;

                    float leftArea = leftBuckets[i].m_bbox.GetSurfaceArea();
                    float rightArea = rightBuckets[i + 1].m_bbox.GetSurfaceArea();

                    float cost = TraversalCost +
                        leftBuckets[i].m_energy * leftArea / bbox.GetSurfaceArea() +
                        rightBuckets[i + 1].m_energy * rightArea / bbox.GetSurfaceArea();

                    if (cost < bestCost)
                    {
                        bestCost = cost;
                        bestAxis = axis;
                        bestBucket = i;
                    }
                }
            }

            return bestCost;
        }

        float LightTreeBuilder::EvaluateSplit(
            const Bucket* leftBuckets, uint32_t leftCount,
            const Bucket* rightBuckets, uint32_t rightCount,
            const Aabb& parentBbox)
        {
            float leftEnergy = 0.0f;
            Aabb leftBbox = Aabb::CreateNull();
            for (uint32_t i = 0; i < leftCount; ++i)
            {
                leftEnergy += leftBuckets[i].m_energy;
                leftBbox.AddAabb(leftBuckets[i].m_bbox);
            }

            float rightEnergy = 0.0f;
            Aabb rightBbox = Aabb::CreateNull();
            for (uint32_t i = 0; i < rightCount; ++i)
            {
                rightEnergy += rightBuckets[i].m_energy;
                rightBbox.AddAabb(rightBuckets[i].m_bbox);
            }

            float parentArea = parentBbox.GetSurfaceArea();
            if (parentArea < 1e-6f)
                return FLT_MAX;

            return TraversalCost +
                leftEnergy * leftBbox.GetSurfaceArea() / parentArea +
                rightEnergy * rightBbox.GetSurfaceArea() / parentArea;
        }

        void LightTreeBuilder::ComputeNodeCone(
            const AZStd::vector<LightTreeInput>& lights,
            AZStd::span<uint32_t> indices,
            float outAxis[3], float& outThetaO)
        {
            if (indices.empty())
            {
                outAxis[0] = 0.0f;
                outAxis[1] = 1.0f;
                outAxis[2] = 0.0f;
                outThetaO = 1.0f;
                return;
            }

            Vector3 avgDir = Vector3::CreateZero();
            for (uint32_t idx : indices)
            {
                avgDir += lights[idx].m_direction;
            }
            avgDir = avgDir.GetNormalized();

            outAxis[0] = avgDir.GetX();
            outAxis[1] = avgDir.GetY();
            outAxis[2] = avgDir.GetZ();

            float maxAngle = 0.0f;
            for (uint32_t idx : indices)
            {
                float d = lights[idx].m_direction.Dot(avgDir);
                d = AZStd::min(AZStd::max(d, -1.0f), 1.0f);
                float angle = std::acos(d);
                if (angle > maxAngle)
                {
                    maxAngle = angle;
                }
            }

            outThetaO = std::cos(maxAngle);
        }
    }
}
