#include <PostProcessing/SsgiPasses.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<SsgiParentPass> SsgiParentPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<SsgiParentPass> pass = aznew SsgiParentPass(descriptor);
            return pass;
        }

        SsgiParentPass::SsgiParentPass(const RPI::PassDescriptor& descriptor)
            : ParentPass(descriptor)
        {
        }

        void SsgiParentPass::InitializeInternal()
        {
            ParentPass::InitializeInternal();
        }

        RPI::Ptr<SsgiComputePass> SsgiComputePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<SsgiComputePass> pass = aznew SsgiComputePass(descriptor);
            return pass;
        }

        SsgiComputePass::SsgiComputePass(const RPI::PassDescriptor& descriptor)
            : ComputePass(descriptor)
        {
        }

        void SsgiComputePass::FrameBeginInternal(FramePrepareParams params)
        {
            ComputePass::FrameBeginInternal(params);

            m_frameIndex = (m_frameIndex + 1) & 63;

            if (m_shaderResourceGroup != nullptr)
            {
                SsgiConstants constants;
                constants.m_frameIndex = m_frameIndex;
                m_shaderResourceGroup->SetConstant(m_constantsIndex, constants);
            }
        }
    }
}
