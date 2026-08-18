#include <CoreLights/RestirPasses.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<RestirParentPass> RestirParentPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<RestirParentPass> pass = aznew RestirParentPass(descriptor);
            return pass;
        }

        RestirParentPass::RestirParentPass(const RPI::PassDescriptor& descriptor)
            : ParentPass(descriptor)
        {
        }

        void RestirParentPass::InitializeInternal()
        {
            ParentPass::InitializeInternal();
        }

        RPI::Ptr<RestirComputePass> RestirComputePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<RestirComputePass> pass = aznew RestirComputePass(descriptor);
            return pass;
        }

        RestirComputePass::RestirComputePass(const RPI::PassDescriptor& descriptor)
            : ComputePass(descriptor)
        {
        }

        void RestirComputePass::FrameBeginInternal(FramePrepareParams params)
        {
            ComputePass::FrameBeginInternal(params);

            m_frameIndex = (m_frameIndex + 1) & 63;

            if (m_shaderResourceGroup != nullptr)
            {
                m_shaderResourceGroup->SetConstant(m_frameIndexIndex, m_frameIndex);
                m_shaderResourceGroup->SetConstant(m_intensityIndex, 1.0f);
                m_shaderResourceGroup->SetConstant(m_debugModeIndex, 0u);
            }
        }
    }
}
