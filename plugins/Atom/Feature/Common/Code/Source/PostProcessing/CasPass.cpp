/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/CasPass.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>

namespace AZ::Render
{

    RPI::Ptr<CasPass> CasPass::Create(const RPI::PassDescriptor& descriptor)
    {
        RPI::Ptr<CasPass> pass = aznew CasPass(descriptor);
        return pass;
    }

    CasPass::CasPass(const RPI::PassDescriptor& descriptor)
        : Base(descriptor)
    {
    }

    void CasPass::SetSharpness(float sharpness)
    {
        m_sharpness = sharpness < 0.0f ? 0.0f : (sharpness > 1.0f ? 1.0f : sharpness);
    }

    void CasPass::BuildInternal()
    {
        m_inputColorBinding = FindAttachmentBinding(Name("InputColor"));
        AZ_Error("CasPass", m_inputColorBinding, "CasPass requires a slot named 'InputColor'.");

        Base::BuildInternal();
    }

    void CasPass::CompileResources(const RHI::FrameGraphCompileContext& context)
    {
        // Must match the Constants struct layout in CasPass.azsl exactly.
        struct CasConstants
        {
            AZStd::array<uint32_t, 2> m_textureSize = { 1, 1 };
            float m_sharpness = 0.5f;
            float m_padding   = 0.0f;
        };

        CasConstants cb;

        if (m_inputColorBinding && m_inputColorBinding->GetAttachment())
        {
            const RHI::Size& size = m_inputColorBinding->GetAttachment()->m_descriptor.m_image.m_size;
            cb.m_textureSize[0] = size.m_width;
            cb.m_textureSize[1] = size.m_height;
        }
        cb.m_sharpness = m_sharpness;

        m_shaderResourceGroup->SetConstant(m_constantDataIndex, cb);

        Base::CompileResources(context);
    }

} // namespace AZ::Render
