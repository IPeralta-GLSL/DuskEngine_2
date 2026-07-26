/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>

namespace AZ::Render
{
    //! CasPass implements FidelityFX Contrast Adaptive Sharpening (sharpen-only, no scaling).
    //! It is designed to run immediately after the TAA pass to restore sharpness lost from
    //! temporal accumulation. Sharpness is configurable in the [0, 1] range.
    class CasPass : public RPI::ComputePass
    {
        using Base = RPI::ComputePass;
        AZ_RPI_PASS(CasPass);

    public:
        AZ_RTTI(AZ::Render::CasPass, "{C1A55F55-8B5D-4A2E-9F1C-0D6E3A7B2C4F}", Base);
        AZ_CLASS_ALLOCATOR(CasPass, SystemAllocator);
        virtual ~CasPass() = default;

        static RPI::Ptr<CasPass> Create(const RPI::PassDescriptor& descriptor);

        //! Set the sharpening strength. 0 = weakest, 1 = strongest. Default is 0.5.
        void SetSharpness(float sharpness);
        float GetSharpness() const { return m_sharpness; }

    private:
        CasPass(const RPI::PassDescriptor& descriptor);

        // RPI::ComputePass overrides
        void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        void BuildInternal() override;

        RHI::ShaderInputNameIndex      m_constantDataIndex  = "m_constantData";
        RPI::PassAttachmentBinding*    m_inputColorBinding  = nullptr;

        float m_sharpness = 0.5f;
    };

} // namespace AZ::Render
