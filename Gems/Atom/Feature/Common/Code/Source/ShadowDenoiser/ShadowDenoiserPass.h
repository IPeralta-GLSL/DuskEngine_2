/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        //! Temporal denoiser pass for ray-traced shadows.
        //! Manages a persistent R16_FLOAT history attachment, applies temporal
        //! accumulation and spatial bilateral filtering to the RT shadow image.
        class ShadowDenoiserPass
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(ShadowDenoiserPass);

        public:
            AZ_RTTI(AZ::Render::ShadowDenoiserPass, "{6B3A7D4E-C920-4F1D-9A2B-3D51F87E0C4A}", RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(AZ::Render::ShadowDenoiserPass, SystemAllocator, 0);

            static RPI::Ptr<ShadowDenoiserPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit ShadowDenoiserPass(const RPI::PassDescriptor& descriptor);

            // Pass overrides
            void BuildInternal() override;
            void FrameEndInternal() override;

            void CreateHistoryAttachmentImage(RPI::Ptr<RPI::PassAttachment>& attachment);

            static constexpr uint32_t ImageFrameCount = 3;
            Data::Instance<RPI::AttachmentImage> m_historyImages[ImageFrameCount];
            uint32_t m_currentImageIndex = 0;

            RPI::PassAttachmentBinding* m_historyBinding = nullptr;
        };
    } // namespace Render
} // namespace AZ
