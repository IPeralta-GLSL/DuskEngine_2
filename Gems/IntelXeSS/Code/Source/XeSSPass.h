#pragma once

#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RHI/Image.h>

#include <xess/xess.h>
#include <xess/xess_vk.h>

namespace AZ::Render
{
    class XeSSPassData
        : public RPI::PassData
    {
    public:
        AZ_RTTI(XeSSPassData, "{D1E2F3A4-B5C6-7D8E-9F0A-1B2C3D4E5F6A}", RPI::PassData);
        AZ_CLASS_ALLOCATOR(XeSSPassData, SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        float m_sharpness = 0.0f;
        int m_qualityMode = static_cast<int>(XESS_QUALITY_SETTING_PERFORMANCE);
    };

    class XeSSPass final
        : public RPI::RenderPass
    {
        using Base = RPI::RenderPass;
        AZ_RPI_PASS(XeSSPass);

    public:
        AZ_RTTI(XeSSPass, "{E2F3A4B5-C6D7-8E9F-0A1B-2C3D4E5F6A7B}", Base);
        AZ_CLASS_ALLOCATOR(XeSSPass, SystemAllocator);

        static RPI::Ptr<XeSSPass> Create(const RPI::PassDescriptor& descriptor);

        void SetQualityMode(xess_quality_settings_t mode);
        void SetSharpness(float sharpness);

    protected:
        explicit XeSSPass(const RPI::PassDescriptor& descriptor);
        ~XeSSPass() override;

        void BuildInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;
        void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;
        void ResetInternal() override;

    private:
        void InitializeXeSSContext();
        void DestroyXeSSContext();
        void UpdateJitterOffset();
        void DestroyImageViews();

        // Halton sequence helper for jitter
        static float HaltonSequence(int index, int base);

        xess_context_handle_t m_xessContext = nullptr;
        bool m_contextCreated = false;
        bool m_needsReset = true;
        bool m_xessAvailable = false;

        xess_quality_settings_t m_qualityMode = XESS_QUALITY_SETTING_PERFORMANCE;
        float m_sharpness = 0.0f;

        uint32_t m_renderWidth = 0;
        uint32_t m_renderHeight = 0;
        uint32_t m_displayWidth = 0;
        uint32_t m_displayHeight = 0;

        float m_jitterX = 0.0f;
        float m_jitterY = 0.0f;
        int32_t m_jitterIndex = 0;

        RPI::PassAttachmentBinding* m_inputColorBinding = nullptr;
        RPI::PassAttachmentBinding* m_inputDepthBinding = nullptr;
        RPI::PassAttachmentBinding* m_motionVectorsBinding = nullptr;
        RPI::PassAttachmentBinding* m_outputColorBinding = nullptr;

        // Cached native handles
        const RHI::Image* m_cachedInputColor = nullptr;
        const RHI::Image* m_cachedInputDepth = nullptr;
        const RHI::Image* m_cachedMotionVectors = nullptr;
        const RHI::Image* m_cachedOutputColor = nullptr;

        // Cached VkImageViews (created/destroyed by this pass)
        VkDevice m_vkDevice = VK_NULL_HANDLE;
        PFN_vkCreateImageView m_pfnCreateImageView = nullptr;
        PFN_vkDestroyImageView m_pfnDestroyImageView = nullptr;
        VkImageView m_inputColorView = VK_NULL_HANDLE;
        VkImageView m_inputDepthView = VK_NULL_HANDLE;
        VkImageView m_motionVectorsView = VK_NULL_HANDLE;
        VkImageView m_outputColorView = VK_NULL_HANDLE;
    };
} // namespace AZ::Render
