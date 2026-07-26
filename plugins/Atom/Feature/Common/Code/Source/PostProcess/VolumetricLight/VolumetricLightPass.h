/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <PostProcess/VolumetricLight/VolumetricLightSettings.h>

namespace AZ
{
    namespace Render
    {
        static const char* const VolumetricLightPassTemplateName = "VolumetricLightPassTemplate";

        class VolumetricLightPass final
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(VolumetricLightPass);

        public:
            AZ_RTTI(VolumetricLightPass, "{A1B2C3D4-E5F6-7890-1234-567890ABCDEF}", RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(VolumetricLightPass, SystemAllocator);
            ~VolumetricLightPass() = default;

            static RPI::Ptr<VolumetricLightPass> Create(const RPI::PassDescriptor& descriptor);

            VolumetricLightSettings* GetPassSettings();
            bool IsEnabled() const override;

        protected:
            VolumetricLightPass(const RPI::PassDescriptor& descriptor);

            void InitializeInternal() override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;

            void SetSrgBindIndices();
            void SetSrgConstants();
            void UpdateEnable(VolumetricLightSettings* settings);

        private:
            VolumetricLightSettings m_fallbackSettings;
            RHI::ShaderInputConstantIndex m_depthTextureDimensionsIndex;
            float m_currentTime;
        };
    }
}
