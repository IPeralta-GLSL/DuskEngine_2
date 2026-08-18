#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <AzCore/Math/Matrix4x4.h>

namespace AZ
{
    namespace Render
    {
        class SsgiParentPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(SsgiParentPass);

        public:
            AZ_RTTI(AZ::Render::SsgiParentPass, "{7C4F1A22-9E53-4B71-8A0D-2F5E93C101AA}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(SsgiParentPass, SystemAllocator);
            virtual ~SsgiParentPass() = default;

            static RPI::Ptr<SsgiParentPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            void InitializeInternal() override;

        private:
            SsgiParentPass(const RPI::PassDescriptor& descriptor);
        };

        class SsgiComputePass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(SsgiComputePass);

        public:
            AZ_RTTI(AZ::Render::SsgiComputePass, "{3D8B7C44-1A69-4F0E-9C2B-6E71D4A8F3B5}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(SsgiComputePass, SystemAllocator);
            virtual ~SsgiComputePass() = default;

            static RPI::Ptr<SsgiComputePass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            SsgiComputePass(const RPI::PassDescriptor& descriptor);

            struct SsgiConstants
            {
                float m_uvRadius = 0.16f;
                float m_falloffMul = 0.03f;
                float m_falloffAdd = -0.3f;
                float m_intensity = 1.0f;
                float m_aoStrength = 0.0f;
                uint32_t m_sliceCount = 2;
                uint32_t m_stepCount = 8;
                uint32_t m_frameIndex = 0;
                float m_padding = 0.0f;
            };

            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
            uint32_t m_frameIndex = 0;
        };
    }
}
