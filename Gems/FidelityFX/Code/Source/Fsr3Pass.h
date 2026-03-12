#pragma once

#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RHI/Image.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wmissing-braces"
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#include <FidelityFX/host/ffx_fsr3upscaler.h>
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wmissing-braces"
#include <FidelityFX/host/ffx_interface.h>
#pragma clang diagnostic pop

namespace AZ::Render
{
    class Fsr3PassData
        : public RPI::PassData
    {
    public:
        AZ_RTTI(Fsr3PassData, "{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}", RPI::PassData);
        AZ_CLASS_ALLOCATOR(Fsr3PassData, SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        float m_sharpness = 0.0f;
        int m_qualityMode = static_cast<int>(FFX_FSR3UPSCALER_QUALITY_MODE_PERFORMANCE);
    };

    class Fsr3Pass final
        : public RPI::RenderPass
    {
        using Base = RPI::RenderPass;
        AZ_RPI_PASS(Fsr3Pass);

    public:
        AZ_RTTI(Fsr3Pass, "{B2C3D4E5-F6A7-8901-BCDE-F12345678901}", Base);
        AZ_CLASS_ALLOCATOR(Fsr3Pass, SystemAllocator);

        static RPI::Ptr<Fsr3Pass> Create(const RPI::PassDescriptor& descriptor);

        void SetQualityMode(FfxFsr3UpscalerQualityMode mode);
        void SetSharpness(float sharpness);

    protected:
        explicit Fsr3Pass(const RPI::PassDescriptor& descriptor);
        ~Fsr3Pass() override;

        void BuildInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;
        void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;
        void ResetInternal() override;

    private:
        void InitializeFsr3Context();
        void DestroyFsr3Context();
        void UpdateJitterOffset();

        FfxFsr3UpscalerContext m_fsrContext{};
        FfxInterface m_backendInterface{};
        void* m_scratchBuffer = nullptr;
        bool m_contextCreated = false;
        bool m_needsReset = true;
        bool m_firstDispatch = false; // true for first dispatch after (re)init to reset temporal history

        FfxFsr3UpscalerQualityMode m_qualityMode = FFX_FSR3UPSCALER_QUALITY_MODE_PERFORMANCE;
        float m_sharpness = 0.0f;
        float m_frameTimeDelta = 16.0f;
        float m_cameraNear = 0.1f;
        float m_cameraFar = 1000.0f;
        float m_cameraFovY = 1.0f;

        uint32_t m_renderWidth = 0;
        uint32_t m_renderHeight = 0;
        uint32_t m_displayWidth = 0;
        uint32_t m_displayHeight = 0;

        float m_jitterX = 0.0f;
        float m_jitterY = 0.0f;
        int32_t m_jitterIndex = 0;
        int32_t m_jitterPhaseCount = 0;

        RPI::PassAttachmentBinding* m_inputColorBinding = nullptr;
        RPI::PassAttachmentBinding* m_inputDepthBinding = nullptr;
        RPI::PassAttachmentBinding* m_motionVectorsBinding = nullptr;
        RPI::PassAttachmentBinding* m_outputColorBinding = nullptr;

        const RHI::Image* m_cachedInputColor = nullptr;
        const RHI::Image* m_cachedInputDepth = nullptr;
        const RHI::Image* m_cachedMotionVectors = nullptr;
        const RHI::Image* m_cachedOutputColor = nullptr;
    };
}
