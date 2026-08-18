#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <AzCore/Math/Matrix4x4.h>

namespace AZ
{
    namespace Render
    {
        class RestirParentPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(RestirParentPass);

        public:
            AZ_RTTI(AZ::Render::RestirParentPass, "{5F2A9B33-7C48-4D15-B3E6-8A70C2D9E11F}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(RestirParentPass, SystemAllocator);
            virtual ~RestirParentPass() = default;

            static RPI::Ptr<RestirParentPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            void InitializeInternal() override;

        private:
            RestirParentPass(const RPI::PassDescriptor& descriptor);
        };

        class RestirComputePass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(RestirComputePass);

        public:
            AZ_RTTI(AZ::Render::RestirComputePass, "{9E1D4C55-2B87-4A26-8D5F-3C91B7A2D6E4}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(RestirComputePass, SystemAllocator);
            virtual ~RestirComputePass() = default;

            static RPI::Ptr<RestirComputePass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            RestirComputePass(const RPI::PassDescriptor& descriptor);

            AZ::RHI::ShaderInputNameIndex m_frameIndexIndex = "m_frameIndex";
            AZ::RHI::ShaderInputNameIndex m_intensityIndex = "m_intensity";
            AZ::RHI::ShaderInputNameIndex m_debugModeIndex = "m_debugMode";
            uint32_t m_frameIndex = 0;
        };
    }
}
