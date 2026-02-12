#include <Application.h>
#include <ProjectUtils.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/Logging/LoggingComponent.h>
#include <AzQtComponents/Utilities/HandleDpiAwareness.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <ProjectManager_Traits_Platform.h>

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>

#if defined(EXTERNAL_CRASH_REPORTING)
#include <ToolsCrashHandler.h>
#endif

namespace O3DE::ProjectManager
{
    Application::~Application()
    {
        TearDown();
    }

    bool Application::Init(bool interactive, AZStd::unique_ptr<PythonBindings> pythonBindings)
    {
#if defined(EXTERNAL_CRASH_REPORTING)
        CrashHandler::ToolsCrashHandler::InitCrashHandler("o3de", {});
#endif

        constexpr const char* applicationName{ "Dusk Engine" };

        QApplication::setOrganizationName(applicationName);
        QApplication::setOrganizationDomain("o3de.org");
        QCoreApplication::setApplicationName(applicationName);
        QCoreApplication::setApplicationVersion("1.0");

        RegisterComponentDescriptor(AzFramework::LogComponent::CreateDescriptor());

        AZ::IO::FixedMaxPath logsPath = AZ::Utils::GetO3deLogsDirectory();
        m_settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_DevWriteStorage, logsPath.LexicallyNormal().Native());
        m_settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::BuildTargetNameKey, applicationName);

        Start(AzFramework::Application::Descriptor());

        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
        QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
        AzQtComponents::Utilities::HandleDpiAwareness(AzQtComponents::Utilities::SystemDpiAware);

        m_app.reset(new QApplication(*GetArgC(), *GetArgV()));

        if (!InitLog(applicationName))
        {
            AZ_Warning("ProjectManager", false, "Failed to init logging");
        }

        QApplication::setWindowIcon(QIcon(":/ProjectManager-Icon.ico"));

        m_pythonBindings = pythonBindings ? AZStd::move(pythonBindings) : AZStd::make_unique<PythonBindings>(GetEngineRoot());

        if (!m_pythonBindings->PythonStarted())
        {
            if (auto result = ProjectUtils::RunGetPythonScript(GetEngineRoot()); result.IsSuccess())
            {
                m_pythonBindings->StartPython();
            }
        }

        if (!m_pythonBindings->PythonStarted())
        {
            if (interactive)
            {
                QMessageBox::critical(nullptr, QObject::tr("Failed to start Python"),
                    QObject::tr("This tool requires an O3DE engine with a Python runtime, "
                                "but was unable to automatically install O3DE's built-in Python."
                                "You can troubleshoot this issue by trying to manually install O3DE's built-in "
                                "Python by running the '%1' script.")
                                .arg(GetPythonScriptPath));
            }
            return false;
        }

        m_settings = AZStd::make_unique<Settings>();

        if (!RegisterEngine(interactive))
        {
            return false;
        }

        const AZ::CommandLine* commandLine = GetCommandLine();
        AZ_Assert(commandLine, "Failed to get command line");

        ProjectManagerScreen startScreen = ProjectManagerScreen::Projects;
        if (size_t screenSwitchCount = commandLine->GetNumSwitchValues("screen"); screenSwitchCount > 0)
        {
            QString screenOption = commandLine->GetSwitchValue("screen", screenSwitchCount - 1).c_str();
            ProjectManagerScreen screen = ProjectUtils::GetProjectManagerScreen(screenOption);
            if (screen != ProjectManagerScreen::Invalid)
            {
                startScreen = screen;
            }
        }

        AZ::IO::FixedMaxPath projectPath;
        if (size_t projectSwitchCount = commandLine->GetNumSwitchValues("project-path"); projectSwitchCount > 0)
        {
            projectPath = commandLine->GetSwitchValue("project-path", projectSwitchCount - 1).c_str();
        }

        m_mainWindow.reset(new ProjectManagerWindow(nullptr, projectPath, startScreen));

        return true;
    }

    bool Application::InitLog(const char* logName)
    {
        if (m_entity)
        {
            return true;
        }

        AZ::IO::FixedMaxPath path = AZ::Utils::GetO3deLogsDirectory();
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "Failed to get FileIOBase instance");

        fileIO->SetAlias("@log@", path.LexicallyNormal().Native().c_str());

        m_entity = aznew AZ::Entity("Application Entity");
        if (m_entity)
        {
            AzFramework::LogComponent* logger = aznew AzFramework::LogComponent();
            AZ_Assert(logger, "Failed to create LogComponent");
            logger->SetLogFileBaseName(logName);
            m_entity->AddComponent(logger);
            m_entity->Init();
            m_entity->Activate();
        }

        return m_entity != nullptr;
    }

    bool Application::RegisterEngine(bool interactive)
    {
        auto engineInfoOutcome = m_pythonBindings->GetEngineInfo();
        if (!engineInfoOutcome)
        {
            if (interactive)
            {
                QMessageBox::critical(nullptr,
                    QObject::tr("Failed to get engine info"),
                    QObject::tr("A valid engine.json could not be found or loaded. "
                                "Please verify a valid engine.json file exists in %1")
                                .arg(GetEngineRoot()));
            }

            AZ_Error("Project Manager", false, "Failed to get engine info");
            return false;
        }

        EngineInfo engineInfo = engineInfoOutcome.GetValue();
        if (engineInfo.m_registered)
        {
            return true;
        }

        constexpr bool forceRegistration = false;
        auto registerOutcome = m_pythonBindings->SetEngineInfo(engineInfo, forceRegistration);
        if (!registerOutcome)
        {
            if (interactive)
            {
                ProjectUtils::DisplayDetailedError(QObject::tr("Failed to register engine"), registerOutcome);
            }

            AZ_Error("Project Manager", false, "Failed to register engine %s : %s",
                engineInfo.m_path.toUtf8().constData(), registerOutcome.GetError().first.c_str());

            return false;
        }

        return true;
    }

    void Application::TearDown()
    {
        if (m_entity)
        {
            m_entity->Deactivate();
            delete m_entity;
            m_entity = nullptr;
        }

        m_pythonBindings.reset();
        m_mainWindow.reset();
        m_app.reset();
    }

    bool Application::Run()
    {
        AzQtComponents::StyleManager styleManager(qApp);
        styleManager.initialize(qApp, GetEngineRoot());

        AZ::IO::FixedMaxPath engineRoot(GetEngineRoot());
        QDir rootDir(engineRoot.c_str());
        const auto pathOnDisk = rootDir.absoluteFilePath("Code/Tools/ProjectManager/Resources");
        const auto qrcPath = QStringLiteral(":/ProjectManager/style");
        AzQtComponents::StyleManager::addSearchPaths("style", pathOnDisk, qrcPath, engineRoot);
        AzQtComponents::StyleManager::setStyleSheet(m_mainWindow.data(), QStringLiteral("style:ProjectManager.qss"));

#if AZ_TRAIT_PROJECT_MANAGER_CUSTOM_TITLEBAR
        auto wrapper = new AzQtComponents::WindowDecorationWrapper();
#else
        auto wrapper = new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionDisabled);
#endif
        wrapper->setGuest(m_mainWindow.data());
        m_mainWindow->show();
        wrapper->enableSaveRestoreGeometry("O3DE", "ProjectManager", "mainWindowGeometry");
        wrapper->showFromSettings();

        qApp->setQuitOnLastWindowClosed(true);

        return qApp->exec();
    }
}
