#include <Fsr3Pass.h>

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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wmissing-braces"
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#include <FidelityFX/host/ffx_fsr3upscaler.h>
#include <FidelityFX/host/backends/vk/ffx_vk.h>
#pragma clang diagnostic pop

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::Render
{
    void Fsr3PassData::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Fsr3PassData, RPI::PassData>()
                ->Version(0)
                ->Field("Sharpness", &Fsr3PassData::m_sharpness)
                ->Field("QualityMode", &Fsr3PassData::m_qualityMode);
        }
    }

    RPI::Ptr<Fsr3Pass> Fsr3Pass::Create(const RPI::PassDescriptor& descriptor)
    {
        RPI::Ptr<Fsr3Pass> pass = aznew Fsr3Pass(descriptor);
        return pass;
    }

    Fsr3Pass::Fsr3Pass(const RPI::PassDescriptor& descriptor)
        : Base(descriptor)
    {
        const Fsr3PassData* passData = RPI::PassUtils::GetPassData<Fsr3PassData>(descriptor);
        if (passData)
        {
            m_sharpness = passData->m_sharpness;
            m_qualityMode = static_cast<FfxFsr3UpscalerQualityMode>(passData->m_qualityMode);
        }
    }

    Fsr3Pass::~Fsr3Pass()
    {
        DestroyFsr3Context();
    }

    void Fsr3Pass::SetQualityMode(FfxFsr3UpscalerQualityMode mode)
    {
        if (m_qualityMode != mode)
        {
            m_qualityMode = mode;
            m_needsReset = true;
        }
    }

    void Fsr3Pass::SetSharpness(float sharpness)
    {
        m_sharpness = AZ::GetClamp(sharpness, 0.0f, 1.0f);
    }

    void Fsr3Pass::InitializeFsr3Context()
    {
        DestroyFsr3Context();

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

        size_t scratchSize = ffxGetScratchMemorySizeVK(vkPhysicalDevice, FFX_FSR3UPSCALER_CONTEXT_COUNT);
        m_scratchBuffer = malloc(scratchSize);

        VkDeviceContext vkDeviceContext{};
        vkDeviceContext.vkDevice = vkDevice;
        vkDeviceContext.vkPhysicalDevice = vkPhysicalDevice;
        vkDeviceContext.vkDeviceProcAddr = AZ::Vulkan::GetDeviceProcAddr(*device);

        FfxDevice ffxDevice = ffxGetDeviceVK(&vkDeviceContext);

        FfxErrorCode result = ffxGetInterfaceVK(
            &m_backendInterface, ffxDevice, m_scratchBuffer, scratchSize, FFX_FSR3UPSCALER_CONTEXT_COUNT);

        if (result != FFX_OK)
        {
            AZ_Error("FidelityFX", false, "Failed to create FidelityFX VK interface: %d", result);
            free(m_scratchBuffer);
            m_scratchBuffer = nullptr;
            return;
        }

        FfxFsr3UpscalerContextDescription contextDesc{};
        contextDesc.flags = FFX_FSR3UPSCALER_ENABLE_HIGH_DYNAMIC_RANGE
            | FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED
            | FFX_FSR3UPSCALER_ENABLE_DEPTH_INFINITE
            | FFX_FSR3UPSCALER_ENABLE_AUTO_EXPOSURE;
        contextDesc.maxRenderSize.width = m_renderWidth;
        contextDesc.maxRenderSize.height = m_renderHeight;
        contextDesc.maxUpscaleSize.width = m_displayWidth;
        contextDesc.maxUpscaleSize.height = m_displayHeight;
        contextDesc.backendInterface = m_backendInterface;
        contextDesc.fpMessage = nullptr;

        result = ffxFsr3UpscalerContextCreate(&m_fsrContext, &contextDesc);

        if (result != FFX_OK)
        {
            AZ_Error("FidelityFX", false, "Failed to create FSR3 Upscaler context: %d", result);
            free(m_scratchBuffer);
            m_scratchBuffer = nullptr;
            return;
        }

        m_contextCreated = true;
        m_jitterPhaseCount = ffxFsr3UpscalerGetJitterPhaseCount(m_renderWidth, m_displayWidth);
        m_jitterIndex = 0;
    }

    void Fsr3Pass::DestroyFsr3Context()
    {
        if (m_contextCreated)
        {
            ffxFsr3UpscalerContextDestroy(&m_fsrContext);
            m_contextCreated = false;
        }

        if (m_scratchBuffer)
        {
            free(m_scratchBuffer);
            m_scratchBuffer = nullptr;
        }
    }

    void Fsr3Pass::UpdateJitterOffset()
    {
        ffxFsr3UpscalerGetJitterOffset(&m_jitterX, &m_jitterY, m_jitterIndex, m_jitterPhaseCount);
        m_jitterIndex = (m_jitterIndex + 1) % m_jitterPhaseCount;
    }

    void Fsr3Pass::BuildInternal()
    {
        Base::BuildInternal();

        m_inputColorBinding = FindAttachmentBinding(Name("InputColor"));
        m_inputDepthBinding = FindAttachmentBinding(Name("InputDepth"));
        m_motionVectorsBinding = FindAttachmentBinding(Name("MotionVectors"));
        m_outputColorBinding = FindAttachmentBinding(Name("OutputColor"));

        if (m_outputColorBinding)
        {
            for (auto& owned : m_ownedAttachments)
            {
                if (owned->m_name == Name("Fsr3Output"))
                {
                    owned->m_sizeSource = nullptr;
                    break;
                }
            }
        }
    }

    void Fsr3Pass::FrameBeginInternal(FramePrepareParams params)
    {
        if (m_outputColorBinding && m_outputColorBinding->GetAttachment())
        {
            if (m_inputColorBinding && m_inputColorBinding->GetAttachment())
            {
                auto inputDesc = m_inputColorBinding->GetAttachment()->m_descriptor.m_image;
                m_renderWidth = inputDesc.m_size.m_width;
                m_renderHeight = inputDesc.m_size.m_height;
            }

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

        if (m_renderWidth == 0 || m_renderHeight == 0 || m_displayWidth == 0 || m_displayHeight == 0)
        {
            Base::FrameBeginInternal(params);
            return;
        }

        if (!m_contextCreated || m_needsReset)
        {
            InitializeFsr3Context();
            m_needsReset = false;
        }

        if (m_contextCreated)
        {
            UpdateJitterOffset();

            RPI::ViewPtr view = GetView();
            if (view)
            {
                float jitterXNdc = 2.0f * m_jitterX / static_cast<float>(m_renderWidth);
                float jitterYNdc = -2.0f * m_jitterY / static_cast<float>(m_renderHeight);
                view->SetClipSpaceOffset(jitterXNdc, jitterYNdc);
            }
        }

        Base::FrameBeginInternal(params);
    }

    void Fsr3Pass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
    {
        Base::SetupFrameGraphDependencies(frameGraph);
    }

    void Fsr3Pass::CompileResources(const RHI::FrameGraphCompileContext& context)
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

    void Fsr3Pass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
    {
        if (!m_contextCreated)
        {
            return;
        }

        if (!m_cachedInputColor || !m_cachedInputDepth ||
            !m_cachedMotionVectors || !m_cachedOutputColor)
        {
            return;
        }

        RHI::CommandList* commandList = context.GetCommandList();
        VkCommandBuffer vkCmdBuffer = AZ::Vulkan::GetNativeCommandBuffer(commandList);

        int deviceIndex = context.GetDeviceIndex();

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

        FfxResourceDescription colorDesc{};
        colorDesc.type = FFX_RESOURCE_TYPE_TEXTURE2D;
        colorDesc.format = FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
        colorDesc.width = m_renderWidth;
        colorDesc.height = m_renderHeight;
        colorDesc.depth = 1;
        colorDesc.mipCount = 1;
        colorDesc.flags = FFX_RESOURCE_FLAGS_NONE;

        FfxResourceDescription depthDesc{};
        depthDesc.type = FFX_RESOURCE_TYPE_TEXTURE2D;
        depthDesc.format = FFX_SURFACE_FORMAT_R32_FLOAT;
        depthDesc.width = m_renderWidth;
        depthDesc.height = m_renderHeight;
        depthDesc.depth = 1;
        depthDesc.mipCount = 1;
        depthDesc.flags = FFX_RESOURCE_FLAGS_NONE;

        FfxResourceDescription motionDesc{};
        motionDesc.type = FFX_RESOURCE_TYPE_TEXTURE2D;
        motionDesc.format = FFX_SURFACE_FORMAT_R16G16_FLOAT;
        motionDesc.width = m_renderWidth;
        motionDesc.height = m_renderHeight;
        motionDesc.depth = 1;
        motionDesc.mipCount = 1;
        motionDesc.flags = FFX_RESOURCE_FLAGS_NONE;

        FfxResourceDescription outputDesc{};
        outputDesc.type = FFX_RESOURCE_TYPE_TEXTURE2D;
        outputDesc.format = FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
        outputDesc.width = m_displayWidth;
        outputDesc.height = m_displayHeight;
        outputDesc.depth = 1;
        outputDesc.mipCount = 1;
        outputDesc.flags = FFX_RESOURCE_FLAGS_NONE;

        FfxFsr3UpscalerDispatchDescription dispatchDesc{};
        dispatchDesc.commandList = ffxGetCommandListVK(vkCmdBuffer);
        dispatchDesc.color = ffxGetResourceVK(colorImage, colorDesc, L"FSR3_InputColor", FFX_RESOURCE_STATE_COMPUTE_READ);
        dispatchDesc.depth = ffxGetResourceVK(depthImage, depthDesc, L"FSR3_InputDepth", FFX_RESOURCE_STATE_COMPUTE_READ);
        dispatchDesc.motionVectors = ffxGetResourceVK(motionImage, motionDesc, L"FSR3_MotionVectors", FFX_RESOURCE_STATE_COMPUTE_READ);
        dispatchDesc.output = ffxGetResourceVK(outputImage, outputDesc, L"FSR3_Output", FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        dispatchDesc.exposure = {};
        dispatchDesc.reactive = {};
        dispatchDesc.transparencyAndComposition = {};
        dispatchDesc.dilatedDepth = {};
        dispatchDesc.dilatedMotionVectors = {};
        dispatchDesc.reconstructedPrevNearestDepth = {};

        dispatchDesc.jitterOffset.x = m_jitterX;
        dispatchDesc.jitterOffset.y = m_jitterY;
        dispatchDesc.motionVectorScale.x = static_cast<float>(m_renderWidth);
        dispatchDesc.motionVectorScale.y = static_cast<float>(m_renderHeight);
        dispatchDesc.renderSize.width = m_renderWidth;
        dispatchDesc.renderSize.height = m_renderHeight;
        dispatchDesc.upscaleSize.width = m_displayWidth;
        dispatchDesc.upscaleSize.height = m_displayHeight;
        dispatchDesc.enableSharpening = m_sharpness > 0.0f;
        dispatchDesc.sharpness = m_sharpness;
        dispatchDesc.frameTimeDelta = AZStd::max(m_frameTimeDelta, 1.0f);
        dispatchDesc.preExposure = 1.0f;
        dispatchDesc.reset = false;
        dispatchDesc.cameraNear = m_cameraNear;
        dispatchDesc.cameraFar = m_cameraFar;
        dispatchDesc.cameraFovAngleVertical = m_cameraFovY;
        dispatchDesc.viewSpaceToMetersFactor = 1.0f;
        dispatchDesc.flags = 0;

        RPI::ViewPtr view = GetView();
        if (view)
        {
            AZ::Matrix4x4 viewToClip = view->GetViewToClipMatrix();

            float m11 = viewToClip(1, 1);
            float m22 = viewToClip(2, 2);
            float m23 = viewToClip(2, 3);

            float nearPlane = 0.0f;
            if (AZ::IsClose(m22, 0.0f, 0.001f))
            {
                nearPlane = m23;
            }
            else
            {
                nearPlane = m23 / m22;
            }

            float fovY = 0.0f;
            if (!AZ::IsClose(m11, 0.0f, 0.001f))
            {
                fovY = 2.0f * atanf(1.0f / m11);
            }

            dispatchDesc.cameraNear = FLT_MAX;
            dispatchDesc.cameraFar = AZ::GetAbs(nearPlane);
            dispatchDesc.cameraFovAngleVertical = AZ::GetAbs(fovY);
        }

        FfxErrorCode result = ffxFsr3UpscalerContextDispatch(&m_fsrContext, &dispatchDesc);
        if (result != FFX_OK)
        {
            AZ_Warning("FidelityFX", false, "FSR3 Upscaler dispatch failed: %d", result);
        }
    }

    void Fsr3Pass::ResetInternal()
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
        Base::ResetInternal();
    }
}
