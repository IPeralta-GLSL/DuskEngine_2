#pragma once

// XeSS dynamic loader — loads libxess at runtime via dlopen/LoadLibrary
// This allows the gem to compile on any platform and gracefully degrade
// if the XeSS runtime is not available.

#include <xess/xess.h>
#include <xess/xess_vk.h>

namespace AZ::Render
{
    // Function pointer types for XeSS API
    using PFN_xessGetVersion = xess_result_t (*)(xess_version_t*);
    using PFN_xessVKCreateContext = xess_result_t (*)(VkInstance, VkPhysicalDevice, VkDevice, xess_context_handle_t*);
    using PFN_xessVKInit = xess_result_t (*)(xess_context_handle_t, const xess_vk_init_params_t*);
    using PFN_xessVKExecute = xess_result_t (*)(xess_context_handle_t, VkCommandBuffer, const xess_vk_execute_params_t*);
    using PFN_xessDestroyContext = xess_result_t (*)(xess_context_handle_t);
    using PFN_xessGetInputResolution = xess_result_t (*)(xess_context_handle_t, const xess_2d_t*, xess_quality_settings_t, xess_2d_t*);
    using PFN_xessGetOptimalInputResolution = xess_result_t (*)(xess_context_handle_t, const xess_2d_t*, xess_quality_settings_t, xess_2d_t*, xess_2d_t*, xess_2d_t*);
    using PFN_xessIsOptimalDriver = xess_result_t (*)(xess_context_handle_t);
    using PFN_xessSetVelocityScale = xess_result_t (*)(xess_context_handle_t, float, float);
    using PFN_xessSetJitterScale = xess_result_t (*)(xess_context_handle_t, float, float);
    using PFN_xessSetLoggingCallback = xess_result_t (*)(xess_context_handle_t, xess_logging_level_t, xess_app_log_callback_t);
    using PFN_xessGetProperties = xess_result_t (*)(xess_context_handle_t, const xess_2d_t*, xess_properties_t*);
    using PFN_xessVKGetRequiredInstanceExtensions = xess_result_t (*)(uint32_t*, const char* const**, uint32_t*);
    using PFN_xessVKBuildPipelines = xess_result_t (*)(xess_context_handle_t, VkPipelineCache, bool, uint32_t);

    class XeSSLoader final
    {
    public:
        static XeSSLoader& Get();

        bool Initialize();
        void Shutdown();
        bool IsAvailable() const { return m_loaded; }

        // XeSS API function pointers
        PFN_xessGetVersion xessGetVersion = nullptr;
        PFN_xessVKCreateContext xessVKCreateContext = nullptr;
        PFN_xessVKInit xessVKInit = nullptr;
        PFN_xessVKExecute xessVKExecute = nullptr;
        PFN_xessDestroyContext xessDestroyContext = nullptr;
        PFN_xessGetInputResolution xessGetInputResolution = nullptr;
        PFN_xessGetOptimalInputResolution xessGetOptimalInputResolution = nullptr;
        PFN_xessIsOptimalDriver xessIsOptimalDriver = nullptr;
        PFN_xessSetVelocityScale xessSetVelocityScale = nullptr;
        PFN_xessSetJitterScale xessSetJitterScale = nullptr;
        PFN_xessSetLoggingCallback xessSetLoggingCallback = nullptr;
        PFN_xessGetProperties xessGetProperties = nullptr;
        PFN_xessVKGetRequiredInstanceExtensions xessVKGetRequiredInstanceExtensions = nullptr;
        PFN_xessVKBuildPipelines xessVKBuildPipelines = nullptr;

    private:
        XeSSLoader() = default;
        ~XeSSLoader();

        void* LoadSymbol(const char* name);

        void* m_library = nullptr;
        bool m_loaded = false;
    };
} // namespace AZ::Render
