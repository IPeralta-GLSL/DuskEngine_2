#pragma once

#if !defined(Q_MOC_RUN)
#include <AzFramework/Application/Application.h>
#include <AzToolsFramework/API/PythonLoader.h>
#include <QCoreApplication>
#include <PythonBindings.h>
#include <Settings.h>
#include <ProjectManagerWindow.h>
#endif

namespace AZ
{
    class Entity;
}

namespace O3DE::ProjectManager
{
    class Application
        : public AzFramework::Application
        , public AzToolsFramework::EmbeddedPython::PythonLoader
    {
    public:
        AZ_CLASS_ALLOCATOR(Application, AZ::SystemAllocator)
        using AzFramework::Application::Application;
        ~Application() override;

        bool Init(bool interactive = true, AZStd::unique_ptr<PythonBindings> pythonBindings = nullptr);
        bool Run();
        void TearDown();

    private:
        bool InitLog(const char* logName);
        bool RegisterEngine(bool interactive);

        QSharedPointer<QCoreApplication> m_app;
        QSharedPointer<ProjectManagerWindow> m_mainWindow;
        AZStd::unique_ptr<PythonBindings> m_pythonBindings;
        AZStd::unique_ptr<Settings> m_settings;
        AZ::Entity* m_entity = nullptr;
    };
}
