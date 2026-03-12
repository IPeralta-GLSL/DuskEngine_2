#include <XeSSPass.h>
#include <XeSSLoader.h>

#include <Atom/RHI/Device.h>
#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/Image.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

#include <Atom/RHI.Interface/Vulkan/RHIVulkanInterface.h>

#include <AzFramework/Windowing/WindowBus.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::Render
{
    // =========================================================================
    // XeSSPassData
    // =========================================================================

    void XeSSPassData::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<XeSSPassData, RPI::PassData>()
                ->Version(0)
                ->Field("Sharpness", &XeSSPassData::m_sharpness)
                ->Field("QualityMode", &XeSSPassData::m_qualityMode);
        }
    }

    // =========================================================================
    // XeSSPass - Construction / Destruction
    // =========================================================================

    RPI::Ptr<XeSSPass> XeSSPass::Create(const RPI::PassDescriptor& descriptor)
    {
        RPI::Ptr<XeSSPass> pass = aznew XeSSPass(descriptor);
        return pass;
    }

    XeSSPass::XeSSPass(const RPI::PassDescriptor& descriptor)
        : Base(descriptor)
    {
        const XeSSPassData* passData = RPI::PassUtils::GetPassData<XeSSPassData>(descriptor);
        if (passData)
        {
            m_sharpness = passData->m_sharpness;
            m_qualityMode = static_cast<xess_quality_settings_t>(passData->m_qualityMode);
        }

        m_xessAvailable = XeSSLoader::Get().IsAvailable();
    }

    XeSSPass::~XeSSPass()
    {
        DestroyXeSSContext();
        DestroyImageViews();
    }

    // =========================================================================
    // XeSSPass - Setters
    // =========================================================================

    void XeSSPass::SetQualityMode(xess_quality_settings_t mode)
    {
        if (m_qualityMode != mode)
        {
            m_qualityMode = mode;
            m_needsReset = true;
        }
    }

    void XeSSPass::SetSharpness(float sharpness)
    {
        m_sharpness = AZ::GetClamp(sharpness, 0.0f, 1.0f);
    }

    // =========================================================================
    // Halton sequence for jitter (standard temporal AA jitter)
    // =========================================================================

    float XeSSPass::HaltonSequence(int index, int base)
    {
        float result = 0.0f;
        float fraction = 1.0f / static_cast<float>(base);
        int i = index;
        while (i > 0)
        {
            result += static_cast<float>(i % base) * fraction;
            i /= base;
            fraction /= static_cast<float>(base);
        }
        return result;
    }

    // =========================================================================
    // Context Management
    // =========================================================================

    void XeSSPass::InitializeXeSSContext()
    {
        DestroyXeSSContext();

        auto& loader = XeSSLoader::Get();
        if (!loader.IsAvailable())
        {
            m_xessAvailable = false;
            return;
        }

        auto* rhiSystem = RHI::RHISystemInterface::Get();
        if (!rhiSystem)
        {
            return;
        }

        RHI::Device* device = rhiSystem->GetDevice();
        if (!device)
        {
            return;
        }

        const RHI::PhysicalDevice& physicalDevice = device->GetPhysicalDevice();

        VkDevice vkDevice = AZ::Vulkan::GetDeviceNativeHandle(*device);
        VkPhysicalDevice vkPhysicalDevice = AZ::Vulkan::GetPhysicalDeviceNativeHandle(physicalDevice);
        VkInstance vkInstance = AZ::Vulkan::GetInstanceNativeHandle();

        m_vkDevice = vkDevice;

        // Get Vulkan function pointers via device proc addr (GLAD doesn't expose global vk* functions)
        PFN_vkGetDeviceProcAddr deviceProcAddr = AZ::Vulkan::GetDeviceProcAddr(*device);
        m_pfnCreateImageView = reinterpret_cast<PFN_vkCreateImageView>(deviceProcAddr(vkDevice, "vkCreateImageView"));
        m_pfnDestroyImageView = reinterpret_cast<PFN_vkDestroyImageView>(deviceProcAddr(vkDevice, "vkDestroyImageView"));

        // Create XeSS context
        xess_result_t result = loader.xessVKCreateContext(
            vkInstance, vkPhysicalDevice, vkDevice, &m_xessContext);
        if (result != XESS_RESULT_SUCCESS)
        {
            AZ_Error("IntelXeSS", false, "Failed to create XeSS context: %d", static_cast<int>(result));
            return;
        }

        // Set velocity scale to render resolution (pixel-space motion vectors)
        if (loader.xessSetVelocityScale)
        {
            loader.xessSetVelocityScale(m_xessContext,
                static_cast<float>(m_renderWidth), static_cast<float>(m_renderHeight));
        }

        // Initialize XeSS
        xess_vk_init_params_t initParams{};
        initParams.outputResolution.x = m_displayWidth;
        initParams.outputResolution.y = m_displayHeight;
        initParams.qualitySetting = m_qualityMode;
        initParams.initFlags = XESS_INIT_FLAG_INVERTED_DEPTH
                             | XESS_INIT_FLAG_ENABLE_AUTOEXPOSURE
                             | XESS_INIT_FLAG_JITTERED_MV;
        initParams.creationNodeMask = 0;
        initParams.visibleNodeMask = 0;
        initParams.tempBufferHeap = VK_NULL_HANDLE;
        initParams.bufferHeapOffset = 0;
        initParams.tempTextureHeap = VK_NULL_HANDLE;
        initParams.textureHeapOffset = 0;
        initParams.pipelineCache = VK_NULL_HANDLE;

        result = loader.xessVKInit(m_xessContext, &initParams);
        if (result != XESS_RESULT_SUCCESS)
        {
            AZ_Error("IntelXeSS", false, "Failed to initialize XeSS: %d", static_cast<int>(result));
            loader.xessDestroyContext(m_xessContext);
            m_xessContext = nullptr;
            return;
        }

        m_contextCreated = true;
        m_firstDispatch = true; // reset temporal history on first dispatch after context (re)init
        m_jitterIndex = 0;

        AZ_TracePrintf("IntelXeSS", "XeSS context initialized: render=%ux%u display=%ux%u quality=%d\n",
            m_renderWidth, m_renderHeight, m_displayWidth, m_displayHeight, static_cast<int>(m_qualityMode));
    }

    void XeSSPass::DestroyXeSSContext()
    {
        if (m_contextCreated && m_xessContext)
        {
            auto& loader = XeSSLoader::Get();
            if (loader.xessDestroyContext)
            {
                loader.xessDestroyContext(m_xessContext);
            }
            m_xessContext = nullptr;
            m_contextCreated = false;
        }
    }

    void XeSSPass::DestroyImageViews()
    {
        if (m_vkDevice == VK_NULL_HANDLE || !m_pfnDestroyImageView)
        {
            return;
        }

        auto DestroyView = [this](VkImageView& view)
        {
            if (view != VK_NULL_HANDLE)
            {
                m_pfnDestroyImageView(m_vkDevice, view, nullptr);
                view = VK_NULL_HANDLE;
            }
        };

        DestroyView(m_inputColorView);
        DestroyView(m_inputDepthView);
        DestroyView(m_motionVectorsView);
        DestroyView(m_outputColorView);
    }

    void XeSSPass::UpdateJitterOffset()
    {
        // Use Halton(2,3) sequence, centered to [-0.5, 0.5] as required by XeSS
        m_jitterIndex++;
        m_jitterX = HaltonSequence(m_jitterIndex, 2) - 0.5f;
        m_jitterY = HaltonSequence(m_jitterIndex, 3) - 0.5f;

        // Wrap index to avoid precision issues after thousands of frames
        if (m_jitterIndex > 512)
        {
            m_jitterIndex = 0;
        }
    }

    // =========================================================================
    // Pass Lifecycle
    // =========================================================================

    void XeSSPass::BuildInternal()
    {
        Base::BuildInternal();

        m_inputColorBinding = FindAttachmentBinding(Name("InputColor"));
        m_inputDepthBinding = FindAttachmentBinding(Name("InputDepth"));
        m_motionVectorsBinding = FindAttachmentBinding(Name("MotionVectors"));
        m_outputColorBinding = FindAttachmentBinding(Name("OutputColor"));

        // Prevent the output from inheriting size from a source — we set it to display resolution
        if (m_outputColorBinding)
        {
            for (auto& owned : m_ownedAttachments)
            {
                if (owned->m_name == Name("XeSSOutput"))
                {
                    owned->m_sizeSource = nullptr;
                    break;
                }
            }
        }
    }

    void XeSSPass::FrameBeginInternal(FramePrepareParams params)
    {
        if (!m_xessAvailable)
        {
            Base::FrameBeginInternal(params);
            return;
        }

        if (m_outputColorBinding && m_outputColorBinding->GetAttachment())
        {
            // Get render resolution from input
            if (m_inputColorBinding && m_inputColorBinding->GetAttachment())
            {
                auto inputDesc = m_inputColorBinding->GetAttachment()->m_descriptor.m_image;
                m_renderWidth = inputDesc.m_size.m_width;
                m_renderHeight = inputDesc.m_size.m_height;
            }

            // Get display resolution from window
            AzFramework::NativeWindowHandle windowHandle = nullptr;
            AzFramework::WindowSystemRequestBus::BroadcastResult(
                windowHandle, &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);

            AzFramework::WindowSize windowSize{};
            if (windowHandle)
            {
                AzFramework::WindowRequestBus::EventResult(
                    windowSize, windowHandle, &AzFramework::WindowRequestBus::Events::GetClientAreaSize);
            }

            if (windowSize.m_width > 0 && windowSize.m_height > 0)
            {
                m_displayWidth = windowSize.m_width;
                m_displayHeight = windowSize.m_height;

                auto& outputAttachment = m_outputColorBinding->GetAttachment();
                outputAttachment->m_descriptor.m_image.m_size.m_width = m_displayWidth;
                outputAttachment->m_descriptor.m_image.m_size.m_height = m_displayHeight;
            }
            else
            {
                auto outputDesc = m_outputColorBinding->GetAttachment()->m_descriptor.m_image;
                m_displayWidth = outputDesc.m_size.m_width;
                m_displayHeight = outputDesc.m_size.m_height;
            }
        }

        if (m_renderWidth < 2 || m_renderHeight < 2 || m_displayWidth < 2 || m_displayHeight < 2)
        {
            // Dimensions not ready yet (e.g. during game mode transition the window
            // temporarily reports 0x0 or 1x1). Skip XeSS init to avoid crashing
            // inside xessVKInit with invalid outputResolution.
            Base::FrameBeginInternal(params);
            return;
        }

        if (!m_contextCreated || m_needsReset)
        {
            InitializeXeSSContext();
            m_needsReset = false;
        }

        if (m_contextCreated)
        {
            UpdateJitterOffset();

            RPI::ViewPtr view = GetView();
            if (view)
            {
                // Convert [-0.5, 0.5] jitter to NDC [-1, 1] space
                float jitterXNdc = 2.0f * m_jitterX / static_cast<float>(m_renderWidth);
                float jitterYNdc = -2.0f * m_jitterY / static_cast<float>(m_renderHeight);
                view->SetClipSpaceOffset(jitterXNdc, jitterYNdc);
            }
        }

        Base::FrameBeginInternal(params);
    }

    void XeSSPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
    {
        Base::SetupFrameGraphDependencies(frameGraph);
    }

    void XeSSPass::CompileResources(const RHI::FrameGraphCompileContext& context)
    {
        auto CacheImage = [&](RPI::PassAttachmentBinding* binding) -> const RHI::Image*
        {
            if (binding && binding->GetAttachment())
            {
                return context.GetImage(binding->GetAttachment()->GetAttachmentId());
            }
            return nullptr;
        };

        m_cachedInputColor = CacheImage(m_inputColorBinding);
        m_cachedInputDepth = CacheImage(m_inputDepthBinding);
        m_cachedMotionVectors = CacheImage(m_motionVectorsBinding);
        m_cachedOutputColor = CacheImage(m_outputColorBinding);
    }

    void XeSSPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
    {
        if (!m_contextCreated || !m_xessAvailable)
        {
            return;
        }

        if (!m_cachedInputColor || !m_cachedInputDepth ||
            !m_cachedMotionVectors || !m_cachedOutputColor)
        {
            return;
        }

        auto& loader = XeSSLoader::Get();
        if (!loader.xessVKExecute)
        {
            return;
        }

        RHI::CommandList* commandList = context.GetCommandList();
        VkCommandBuffer vkCmdBuffer = AZ::Vulkan::GetNativeCommandBuffer(commandList);

        int deviceIndex = context.GetDeviceIndex();

        // Helper to get VkImage from RHI::Image
        auto GetVkImage = [&](const RHI::Image* image) -> VkImage
        {
            auto deviceImage = image->GetDeviceImage(deviceIndex);
            if (!deviceImage)
            {
                return VK_NULL_HANDLE;
            }
            return AZ::Vulkan::GetNativeImage(*deviceImage);
        };

        VkImage colorImage = GetVkImage(m_cachedInputColor);
        VkImage depthImage = GetVkImage(m_cachedInputDepth);
        VkImage motionImage = GetVkImage(m_cachedMotionVectors);
        VkImage outputImage = GetVkImage(m_cachedOutputColor);

        if (colorImage == VK_NULL_HANDLE || depthImage == VK_NULL_HANDLE ||
            motionImage == VK_NULL_HANDLE || outputImage == VK_NULL_HANDLE)
        {
            return;
        }

        // Destroy old image views and create new ones matching current images
        DestroyImageViews();

        if (!m_pfnCreateImageView)
        {
            return;
        }

        auto CreateView = [this](VkImage image, VkFormat format, VkImageAspectFlags aspect,
                                 uint32_t width, uint32_t height) -> VkImageView
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspect;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView view = VK_NULL_HANDLE;
            VkResult result = m_pfnCreateImageView(m_vkDevice, &viewInfo, nullptr, &view);
            if (result != VK_SUCCESS)
            {
                AZ_Warning("IntelXeSS", false, "Failed to create VkImageView for XeSS: %d", result);
                return VK_NULL_HANDLE;
            }
            return view;
        };

        m_inputColorView = CreateView(colorImage, VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_ASPECT_COLOR_BIT, m_renderWidth, m_renderHeight);
        m_inputDepthView = CreateView(depthImage, VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_ASPECT_DEPTH_BIT, m_renderWidth, m_renderHeight);
        m_motionVectorsView = CreateView(motionImage, VK_FORMAT_R16G16_SFLOAT,
            VK_IMAGE_ASPECT_COLOR_BIT, m_renderWidth, m_renderHeight);
        m_outputColorView = CreateView(outputImage, VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_ASPECT_COLOR_BIT, m_displayWidth, m_displayHeight);

        if (m_inputColorView == VK_NULL_HANDLE || m_inputDepthView == VK_NULL_HANDLE ||
            m_motionVectorsView == VK_NULL_HANDLE || m_outputColorView == VK_NULL_HANDLE)
        {
            DestroyImageViews();
            return;
        }

        // Build XeSS execute params
        VkImageSubresourceRange colorSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        VkImageSubresourceRange depthSubresource = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

        xess_vk_execute_params_t execParams{};

        execParams.colorTexture.imageView = m_inputColorView;
        execParams.colorTexture.image = colorImage;
        execParams.colorTexture.subresourceRange = colorSubresource;
        execParams.colorTexture.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        execParams.colorTexture.width = m_renderWidth;
        execParams.colorTexture.height = m_renderHeight;

        execParams.depthTexture.imageView = m_inputDepthView;
        execParams.depthTexture.image = depthImage;
        execParams.depthTexture.subresourceRange = depthSubresource;
        execParams.depthTexture.format = VK_FORMAT_D32_SFLOAT;
        execParams.depthTexture.width = m_renderWidth;
        execParams.depthTexture.height = m_renderHeight;

        execParams.velocityTexture.imageView = m_motionVectorsView;
        execParams.velocityTexture.image = motionImage;
        execParams.velocityTexture.subresourceRange = colorSubresource;
        execParams.velocityTexture.format = VK_FORMAT_R16G16_SFLOAT;
        execParams.velocityTexture.width = m_renderWidth;
        execParams.velocityTexture.height = m_renderHeight;

        execParams.outputTexture.imageView = m_outputColorView;
        execParams.outputTexture.image = outputImage;
        execParams.outputTexture.subresourceRange = colorSubresource;
        execParams.outputTexture.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        execParams.outputTexture.width = m_displayWidth;
        execParams.outputTexture.height = m_displayHeight;

        // Optional textures — not used
        execParams.exposureScaleTexture.image = VK_NULL_HANDLE;
        execParams.exposureScaleTexture.imageView = VK_NULL_HANDLE;
        execParams.responsivePixelMaskTexture.image = VK_NULL_HANDLE;
        execParams.responsivePixelMaskTexture.imageView = VK_NULL_HANDLE;

        execParams.jitterOffsetX = m_jitterX;
        execParams.jitterOffsetY = m_jitterY;
        execParams.exposureScale = 1.0f;
        execParams.resetHistory = m_firstDispatch ? 1 : 0;
        m_firstDispatch = false;
        execParams.inputWidth = m_renderWidth;
        execParams.inputHeight = m_renderHeight;

        // Execute XeSS upscaling
        xess_result_t result = loader.xessVKExecute(m_xessContext, vkCmdBuffer, &execParams);
        if (result != XESS_RESULT_SUCCESS)
        {
            AZ_Warning("IntelXeSS", false, "XeSS execute failed: %d", static_cast<int>(result));
        }
    }

    void XeSSPass::ResetInternal()
    {
        m_inputColorBinding = nullptr;
        m_inputDepthBinding = nullptr;
        m_motionVectorsBinding = nullptr;
        m_outputColorBinding = nullptr;
        m_cachedInputColor = nullptr;
        m_cachedInputDepth = nullptr;
        m_cachedMotionVectors = nullptr;
        m_cachedOutputColor = nullptr;
        m_needsReset = true;
        DestroyImageViews();
        Base::ResetInternal();
    }
} // namespace AZ::Render
