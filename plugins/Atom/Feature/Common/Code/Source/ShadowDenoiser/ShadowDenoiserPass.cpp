/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "ShadowDenoiserPass.h"

#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ShadowDenoiserPass> ShadowDenoiserPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<ShadowDenoiserPass> pass = aznew ShadowDenoiserPass(descriptor);
            return AZStd::move(pass);
        }

        ShadowDenoiserPass::ShadowDenoiserPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void ShadowDenoiserPass::CreateHistoryAttachmentImage(RPI::Ptr<RPI::PassAttachment>& attachment)
        {
            Data::Instance<RPI::AttachmentImagePool> pool =
                RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

            m_currentImageIndex = (m_currentImageIndex + 1) % ImageFrameCount;

            RHI::ImageViewDescriptor viewDesc = RHI::ImageViewDescriptor::Create(RHI::Format::R16G16B16A16_FLOAT, 0, 0);
            RHI::ClearValue clearValue = RHI::ClearValue::CreateVector4Float(1.0f, 0.0f, 0.0f, 0.0f);

            m_historyImages[m_currentImageIndex] = RPI::AttachmentImage::Create(
                *pool.get(),
                attachment->m_descriptor.m_image,
                Name(attachment->m_path.GetCStr()),
                &clearValue,
                &viewDesc);

            attachment->m_importedResource = m_historyImages[m_currentImageIndex];
        }

        void ShadowDenoiserPass::BuildInternal()
        {
            // The first owned attachment is the pass output (created from ImageAttachments in the template).
            // We match its dimensions to allocate the history texture.
            AZ_Assert(!m_ownedAttachments.empty(),
                "ShadowDenoiserPass: pass template must define at least one ImageAttachment for the output");

            RHI::ImageDescriptor outputDesc = m_ownedAttachments[0]->m_descriptor.m_image;

            // Create history attachment (R16G16B16A16_FLOAT, same dimensions as output)
            RHI::ImageBindFlags bindFlags = RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderReadWrite;
            RHI::ImageDescriptor histDesc = RHI::ImageDescriptor::Create2D(
                bindFlags,
                outputDesc.m_size.m_width,
                outputDesc.m_size.m_height,
                RHI::Format::R16G16B16A16_FLOAT);

            RPI::Ptr<RPI::PassAttachment> histAttachment = aznew RPI::PassAttachment();
            AZStd::string histName = AZStd::string::format("%s.ShadowHistory", GetPathName().GetCStr());
            histAttachment->m_name      = histName;
            histAttachment->m_path      = histName;
            histAttachment->m_lifetime  = RHI::AttachmentLifetimeType::Imported;
            histAttachment->m_descriptor = histDesc;
            m_ownedAttachments.push_back(histAttachment);

            CreateHistoryAttachmentImage(histAttachment);

            m_historyBinding = FindAttachmentBinding(AZ::Name("History"));
            if (m_historyBinding)
            {
                m_historyBinding->SetAttachment(histAttachment);
            }
            else
            {
                AZ_Error("ShadowDenoiserPass", false,
                    "ShadowDenoiserPass requires a slot named 'History' in its pass template");
            }

            FullscreenTrianglePass::BuildInternal();
        }

        void ShadowDenoiserPass::FrameEndInternal()
        {
            if (m_historyBinding)
            {
                RPI::Ptr<RPI::PassAttachment> histAttachment = m_historyBinding->GetAttachment();
                if (histAttachment)
                {
                    histAttachment->Update();

                    // If the render target resized, recreate history image at new size
                    RHI::Size& histSize   = histAttachment->m_descriptor.m_image.m_size;
                    RHI::Size& outputSize = m_ownedAttachments[0]->m_descriptor.m_image.m_size;
                    if (histSize != outputSize)
                    {
                        histSize = outputSize;
                        CreateHistoryAttachmentImage(histAttachment);
                        m_historyBinding->SetAttachment(histAttachment);
                    }
                }
            }

            FullscreenTrianglePass::FrameEndInternal();
        }

    } // namespace Render
} // namespace AZ
