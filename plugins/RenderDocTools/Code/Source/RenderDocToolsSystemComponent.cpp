#include "RenderDocToolsSystemComponent.h"

#include "RenderDocToolbarWidget.h"

#include <renderdoc_app.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorToolBarIdentifiers.h>

#include <QLibrary>
#include <QToolBar>

namespace AZ::Render
{
    namespace
    {
        constexpr char LogName[] = "RenderDocTools";
        constexpr char WidgetActionIdentifier[] = "o3de.widgetAction.editor.renderdoc";

        typedef uint32_t (*RenderDocGetAPIFunc)(RENDERDOC_Version version, void** outAPIPointers);
    }

    void RenderDocToolsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RenderDocToolsSystemComponent, AZ::Component>()->Version(0);
        }
    }

    void RenderDocToolsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RenderDocToolsService"));
    }

    void RenderDocToolsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RenderDocToolsService"));
    }

    void RenderDocToolsSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void RenderDocToolsSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void RenderDocToolsSystemComponent::Activate()
    {
        LoadRenderDocLibrary();

        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    void RenderDocToolsSystemComponent::Deactivate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
    }

    void RenderDocToolsSystemComponent::LoadRenderDocLibrary()
    {
        if (m_libraryLoadAttempted)
        {
            return;
        }
        m_libraryLoadAttempted = true;

        QLibrary lib;
        const char* candidates[] = {
            "librenderdoc.so",
            "/usr/lib/librenderdoc.so",
            "/usr/local/lib/librenderdoc.so",
            "renderdoc",
        };

        for (const char* candidate : candidates)
        {
            lib.setFileName(QString(candidate));
            if (lib.load())
            {
                break;
            }
        }

        if (!lib.isLoaded())
        {
            AZ_Warning(LogName, false, "Could not load librenderdoc.so; RenderDoc captures will be unavailable in this session.");
            return;
        }

        auto getApi = reinterpret_cast<RenderDocGetAPIFunc>(lib.resolve("RENDERDOC_GetAPI"));
        if (!getApi)
        {
            AZ_Warning(LogName, false, "librenderdoc.so loaded but RENDERDOC_GetAPI was not found.");
            return;
        }

        constexpr RENDERDOC_Version versions[] = {
            eRENDERDOC_API_Version_1_7_0,
            eRENDERDOC_API_Version_1_6_0,
            eRENDERDOC_API_Version_1_5_0,
            eRENDERDOC_API_Version_1_4_2,
            eRENDERDOC_API_Version_1_4_1,
            eRENDERDOC_API_Version_1_4_0,
            eRENDERDOC_API_Version_1_3_0,
            eRENDERDOC_API_Version_1_2_0,
            eRENDERDOC_API_Version_1_1_2,
        };

        for (RENDERDOC_Version version : versions)
        {
            void* api = nullptr;
            if (getApi(version, &api) == 1 && api)
            {
                m_renderdocApi = static_cast<RENDERDOC_API_1_7_0*>(api);
                AZ_Printf(LogName, "RenderDoc in-app API loaded (version %d).", static_cast<int>(version));
                return;
            }
        }

        AZ_Warning(LogName, false, "RENDERDOC_GetAPI did not provide any supported API version.");
    }

    void RenderDocToolsSystemComponent::OnWidgetActionRegistrationHook()
    {
        auto* actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        if (!actionManagerInterface)
        {
            return;
        }

        AzToolsFramework::WidgetActionProperties widgetActionProperties;
        widgetActionProperties.m_name = "RenderDoc Capture";
        widgetActionProperties.m_category = "Rendering";

        RENDERDOC_API_1_7_0* renderdocApi = m_renderdocApi;
        actionManagerInterface->RegisterWidgetAction(
            WidgetActionIdentifier,
            widgetActionProperties,
            [renderdocApi]() -> QWidget*
            {
                return new RenderDocToolbarWidget(renderdocApi);
            });
    }

    void RenderDocToolsSystemComponent::OnToolBarBindingHook()
    {
        auto* toolBarManagerInterface = AZ::Interface<AzToolsFramework::ToolBarManagerInterface>::Get();
        if (!toolBarManagerInterface)
        {
            return;
        }

        toolBarManagerInterface->AddWidgetToToolBar(
            AZStd::string(EditorIdentifiers::ToolsToolBarIdentifier), WidgetActionIdentifier, 1900);
    }
}
