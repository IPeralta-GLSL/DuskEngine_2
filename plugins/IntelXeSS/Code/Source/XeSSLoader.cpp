#include <XeSSLoader.h>
#include <AzCore/Debug/Trace.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

namespace AZ::Render
{
    XeSSLoader& XeSSLoader::Get()
    {
        static XeSSLoader instance;
        return instance;
    }

    XeSSLoader::~XeSSLoader()
    {
        Shutdown();
    }

    void* XeSSLoader::LoadSymbol(const char* name)
    {
        if (!m_library)
        {
            return nullptr;
        }

#ifdef _WIN32
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(m_library), name));
#else
        return dlsym(m_library, name);
#endif
    }

    bool XeSSLoader::Initialize()
    {
        if (m_loaded)
        {
            return true;
        }

        // Try to load the XeSS shared library
#ifdef _WIN32
        const char* libNames[] = { "libxess.dll", nullptr };
#else
        const char* libNames[] = { "libxess.so", "libxess.so.1", "libxess.so.2", nullptr };
#endif

        for (int i = 0; libNames[i] != nullptr; ++i)
        {
#ifdef _WIN32
            m_library = static_cast<void*>(LoadLibraryA(libNames[i]));
#else
            m_library = dlopen(libNames[i], RTLD_NOW | RTLD_LOCAL);
#endif
            if (m_library)
            {
                AZ_TracePrintf("IntelXeSS", "Loaded XeSS library: %s\n", libNames[i]);
                break;
            }
        }

        if (!m_library)
        {
            AZ_Warning("IntelXeSS", false,
                "Intel XeSS library not found. XeSS upscaling will be unavailable. "
                "Ensure libxess.so (Linux) or libxess.dll (Windows) is in the library search path.");
            return false;
        }

        // Load all function pointers
#define LOAD_XESS_FUNC(func) \
    func = reinterpret_cast<PFN_##func>(LoadSymbol(#func)); \
    if (!func) { AZ_Warning("IntelXeSS", false, "Failed to load XeSS function: %s", #func); }

        LOAD_XESS_FUNC(xessGetVersion);
        LOAD_XESS_FUNC(xessVKCreateContext);
        LOAD_XESS_FUNC(xessVKInit);
        LOAD_XESS_FUNC(xessVKExecute);
        LOAD_XESS_FUNC(xessDestroyContext);
        LOAD_XESS_FUNC(xessGetInputResolution);
        LOAD_XESS_FUNC(xessGetOptimalInputResolution);
        LOAD_XESS_FUNC(xessIsOptimalDriver);
        LOAD_XESS_FUNC(xessSetVelocityScale);
        LOAD_XESS_FUNC(xessSetJitterScale);
        LOAD_XESS_FUNC(xessSetLoggingCallback);
        LOAD_XESS_FUNC(xessGetProperties);
        LOAD_XESS_FUNC(xessVKGetRequiredInstanceExtensions);
        LOAD_XESS_FUNC(xessVKBuildPipelines);

#undef LOAD_XESS_FUNC

        // Verify critical functions loaded
        if (!xessVKCreateContext || !xessVKInit || !xessVKExecute || !xessDestroyContext)
        {
            AZ_Error("IntelXeSS", false, "Critical XeSS functions not found. Library may be incompatible.");
            Shutdown();
            return false;
        }

        // Log version if available
        if (xessGetVersion)
        {
            xess_version_t ver{};
            if (xessGetVersion(&ver) == XESS_RESULT_SUCCESS)
            {
                AZ_TracePrintf("IntelXeSS", "Intel XeSS version: %d.%d.%d\n", ver.major, ver.minor, ver.patch);
            }
        }

        m_loaded = true;
        return true;
    }

    void XeSSLoader::Shutdown()
    {
        if (m_library)
        {
#ifdef _WIN32
            FreeLibrary(static_cast<HMODULE>(m_library));
#else
            dlclose(m_library);
#endif
            m_library = nullptr;
        }

        m_loaded = false;

        xessGetVersion = nullptr;
        xessVKCreateContext = nullptr;
        xessVKInit = nullptr;
        xessVKExecute = nullptr;
        xessDestroyContext = nullptr;
        xessGetInputResolution = nullptr;
        xessGetOptimalInputResolution = nullptr;
        xessIsOptimalDriver = nullptr;
        xessSetVelocityScale = nullptr;
        xessSetJitterScale = nullptr;
        xessSetLoggingCallback = nullptr;
        xessGetProperties = nullptr;
        xessVKGetRequiredInstanceExtensions = nullptr;
        xessVKBuildPipelines = nullptr;
    }
} // namespace AZ::Render
