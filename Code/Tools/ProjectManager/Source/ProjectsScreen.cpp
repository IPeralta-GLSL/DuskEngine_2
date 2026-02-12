#include <ProjectsScreen.h>

#include <AddRemoteProjectDialog.h>
#include <ProjectBuilderController.h>
#include <ProjectButtonWidget.h>
#include <ProjectExportController.h>
#include <ProjectManagerDefs.h>
#include <ProjectUtils.h>
#include <PythonBindings.h>
#include <PythonBindingsInterface.h>
#include <ScreensCtrl.h>
#include <SettingsInterface.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/Platform.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/sort.h>
#include <AzFramework/AzFramework_Traits_Platform.h>
#include <AzFramework/Process/ProcessCommon.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzQtComponents/Components/FlowLayout.h>

#include <QDir>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListView>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QPushButton>
#include <QQueue>
#include <QScrollArea>
#include <QSettings>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace O3DE::ProjectManager
{
    ProjectsScreen::ProjectsScreen(DownloadController* downloadController, QWidget* parent)
        : ScreenWidget(parent)
        , m_downloadController(downloadController)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setAlignment(Qt::AlignTop);
        vLayout->setContentsMargins(s_contentMargins, 0, s_contentMargins, 20);
        setLayout(vLayout);

        m_fileSystemWatcher = new QFileSystemWatcher(this);
        connect(m_fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, &ProjectsScreen::HandleProjectFilePathChanged);

        m_stack = new QStackedWidget(this);
        m_firstTimeContent = CreateFirstTimeContent();
        m_stack->addWidget(m_firstTimeContent);
        m_projectsContent = CreateProjectsContent();
        m_stack->addWidget(m_projectsContent);
        vLayout->addWidget(m_stack);

        connect(static_cast<ScreensCtrl*>(parent), &ScreensCtrl::NotifyBuildProject, this, &ProjectsScreen::SuggestBuildProject);
        connect(m_downloadController, &DownloadController::Done, this, &ProjectsScreen::HandleDownloadResult);
        connect(m_downloadController, &DownloadController::ObjectDownloadProgress, this, &ProjectsScreen::HandleDownloadProgress);
    }

    ProjectsScreen::~ProjectsScreen() = default;

    ProjectManagerScreen ProjectsScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::Projects;
    }

    bool ProjectsScreen::IsTab()
    {
        return true;
    }

    QString ProjectsScreen::GetTabText()
    {
        return tr("Projects");
    }

    QFrame* ProjectsScreen::CreateFirstTimeContent()
    {
        QFrame* frame = new QFrame(this);
        frame->setObjectName("firstTimeContent");

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setAlignment(Qt::AlignTop);
        frame->setLayout(layout);

        QLabel* titleLabel = new QLabel(tr("Ready? Set. Create!"), this);
        titleLabel->setObjectName("titleLabel");
        layout->addWidget(titleLabel);

        QLabel* introLabel = new QLabel(this);
        introLabel->setObjectName("introLabel");
        introLabel->setText(tr("Welcome to Dusk Engine! Start something new by creating a project."));
        layout->addWidget(introLabel);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setAlignment(Qt::AlignLeft);
        buttonLayout->setSpacing(s_spacerSize);

        QPushButton* createProjectButton = new QPushButton(tr("Create a project\n"), this);
        createProjectButton->setObjectName("createProjectButton");
        buttonLayout->addWidget(createProjectButton);

        QPushButton* addProjectButton = new QPushButton(tr("Open a project\n"), this);
        addProjectButton->setObjectName("addProjectButton");
        buttonLayout->addWidget(addProjectButton);

        QPushButton* addRemoteProjectButton = new QPushButton(tr("Add a remote project\n"), this);
        addRemoteProjectButton->setObjectName("addRemoteProjectButton");
        buttonLayout->addWidget(addRemoteProjectButton);

        connect(createProjectButton, &QPushButton::clicked, this, &ProjectsScreen::HandleNewProjectButton);
        connect(addProjectButton, &QPushButton::clicked, this, &ProjectsScreen::HandleAddProjectButton);
        connect(addRemoteProjectButton, &QPushButton::clicked, this, &ProjectsScreen::HandleAddRemoteProjectButton);

        layout->addLayout(buttonLayout);

        return frame;
    }

    QFrame* ProjectsScreen::CreateProjectsContent()
    {
        QFrame* frame = new QFrame(this);
        frame->setObjectName("projectsContent");

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignTop);
        layout->setContentsMargins(0, 0, 0, 0);
        frame->setLayout(layout);

        QFrame* header = new QFrame(frame);
        QHBoxLayout* headerLayout = new QHBoxLayout();

        QLabel* titleLabel = new QLabel(tr("My Projects"), this);
        titleLabel->setObjectName("titleLabel");
        headerLayout->addWidget(titleLabel);

        QMenu* newProjectMenu = new QMenu(this);
        m_createNewProjectAction = newProjectMenu->addAction("Create New Project");
        m_addExistingProjectAction = newProjectMenu->addAction("Open Existing Project");
        m_addRemoteProjectAction = newProjectMenu->addAction("Add a Remote Project");

        connect(m_createNewProjectAction, &QAction::triggered, this, &ProjectsScreen::HandleNewProjectButton);
        connect(m_addExistingProjectAction, &QAction::triggered, this, &ProjectsScreen::HandleAddProjectButton);
        connect(m_addRemoteProjectAction, &QAction::triggered, this, &ProjectsScreen::HandleAddRemoteProjectButton);

        QPushButton* newProjectMenuButton = new QPushButton(tr("New Project..."), this);
        newProjectMenuButton->setObjectName("newProjectButton");
        newProjectMenuButton->setMenu(newProjectMenu);
        newProjectMenuButton->setDefault(true);
        headerLayout->addWidget(newProjectMenuButton);

        header->setLayout(headerLayout);
        layout->addWidget(header);

        QScrollArea* projectsScrollArea = new QScrollArea(this);
        QWidget* scrollWidget = new QWidget();
        m_projectsFlowLayout = new FlowLayout(0, s_spacerSize, s_spacerSize);
        scrollWidget->setLayout(m_projectsFlowLayout);
        projectsScrollArea->setWidget(scrollWidget);
        projectsScrollArea->setWidgetResizable(true);
        layout->addWidget(projectsScrollArea);

        return frame;
    }

    ProjectButton* ProjectsScreen::CreateProjectButton(const ProjectInfo& project, const EngineInfo& engine)
    {
        ProjectButton* projectButton = new ProjectButton(project, engine, this);
        m_projectButtons.insert({ project.m_path.toUtf8().constData(), projectButton });
        m_projectsFlowLayout->addWidget(projectButton);

        connect(projectButton, &ProjectButton::OpenProject, this, &ProjectsScreen::HandleOpenProject);
        connect(projectButton, &ProjectButton::EditProject, this, &ProjectsScreen::HandleEditProject);
        connect(projectButton, &ProjectButton::EditProjectGems, this, &ProjectsScreen::HandleEditProjectGems);
        connect(projectButton, &ProjectButton::CopyProject, this, &ProjectsScreen::HandleCopyProject);
        connect(projectButton, &ProjectButton::RemoveProject, this, &ProjectsScreen::HandleRemoveProject);
        connect(projectButton, &ProjectButton::DeleteProject, this, &ProjectsScreen::HandleDeleteProject);
        connect(projectButton, &ProjectButton::BuildProject, this, &ProjectsScreen::QueueBuildProject);
        connect(projectButton, &ProjectButton::ExportProject, this, &ProjectsScreen::QueueExportProject);
        connect(projectButton, &ProjectButton::OpenCMakeGUI, this,
            [this](const ProjectInfo& projectInfo)
            {
                AZ::Outcome result = ProjectUtils::OpenCMakeGUI(projectInfo.m_path);
                if (!result)
                {
                    QMessageBox::critical(this, tr("Failed to open CMake GUI"), result.GetError(), QMessageBox::Ok);
                }
            });
        connect(projectButton, &ProjectButton::OpenAndroidProjectGenerator, this, &ProjectsScreen::HandleOpenAndroidProjectGenerator);
        connect(projectButton, &ProjectButton::OpenProjectExportSettings, this, &ProjectsScreen::HandleOpenProjectExportSettings);

        return projectButton;
    }

    void ProjectsScreen::NotifyCurrentScreen()
    {
        const QVector<ProjectInfo>& projects = GetAllProjects();
        const bool projectsFound = !projects.isEmpty();

        if (ShouldDisplayFirstTimeContent(projectsFound))
        {
            m_background.load(":/Backgrounds/FtueBackground.jpg");
            m_stack->setCurrentWidget(m_firstTimeContent);
        }
        else
        {
            m_background.load(":/Backgrounds/DefaultBackground.jpg");
            UpdateWithProjects(projects);
        }
    }

    bool ProjectsScreen::ShouldDisplayFirstTimeContent(bool projectsFound)
    {
        if (projectsFound)
        {
            return false;
        }

        QSettings settings;
        bool displayFirstTimeContent = settings.value("displayFirstTimeContent", true).toBool();
        if (displayFirstTimeContent)
        {
            settings.setValue("displayFirstTimeContent", false);
        }

        return displayFirstTimeContent;
    }

    QVector<ProjectInfo> ProjectsScreen::GetAllProjects()
    {
        QVector<ProjectInfo> projects;

        auto projectsResult = PythonBindingsInterface::Get()->GetProjects();
        if (projectsResult.IsSuccess() && !projectsResult.GetValue().isEmpty())
        {
            projects.append(projectsResult.GetValue());
        }

        auto remoteProjectsResult = PythonBindingsInterface::Get()->GetProjectsForAllRepos();
        if (remoteProjectsResult.IsSuccess() && !remoteProjectsResult.GetValue().isEmpty())
        {
            for (const ProjectInfo& remoteProject : remoteProjectsResult.TakeValue())
            {
                auto foundProject = AZStd::ranges::find_if(projects,
                    [&remoteProject](const ProjectInfo& value)
                    {
                        return remoteProject.m_id == value.m_id;
                    });
                if (foundProject == projects.end())
                {
                    projects.append(remoteProject);
                }
            }
        }

        AZ::IO::Path buildProjectPath;
        if (m_currentBuilder)
        {
            buildProjectPath = AZ::IO::Path(m_currentBuilder->GetProjectInfo().m_path.toUtf8().constData());
        }

        AZStd::sort(projects.begin(), projects.end(),
            [buildProjectPath, this](const ProjectInfo& arg1, const ProjectInfo& arg2)
            {
                if (!buildProjectPath.empty())
                {
                    if (AZ::IO::Path(arg1.m_path.toUtf8().constData()) == buildProjectPath)
                    {
                        return true;
                    }
                    if (AZ::IO::Path(arg2.m_path.toUtf8().constData()) == buildProjectPath)
                    {
                        return false;
                    }
                }

                bool arg1InBuildQueue = BuildQueueContainsProject(arg1.m_path);
                bool arg2InBuildQueue = BuildQueueContainsProject(arg2.m_path);

                if (arg1InBuildQueue && !arg2InBuildQueue)
                {
                    return true;
                }
                if (!arg1InBuildQueue && arg2InBuildQueue)
                {
                    return false;
                }
                if (arg1.m_displayName.compare(arg2.m_displayName, Qt::CaseInsensitive) == 0)
                {
                    return arg1.m_path.toLower() < arg2.m_path.toLower();
                }
                return arg1.m_displayName.toLower() < arg2.m_displayName.toLower();
            });

        return projects;
    }

    void ProjectsScreen::UpdateIfCurrentScreen()
    {
        if (IsCurrentScreen())
        {
            UpdateWithProjects(GetAllProjects());
        }
    }

    void ProjectsScreen::RemoveProjectButtonsFromFlowLayout(const QVector<ProjectInfo>& projectsToKeep)
    {
        AZStd::unordered_set<AZ::IO::Path> keepProject;
        for (const ProjectInfo& project : projectsToKeep)
        {
            keepProject.insert(project.m_path.toUtf8().constData());
        }

        auto projectButtonsIter = m_projectButtons.begin();
        while (projectButtonsIter != m_projectButtons.end())
        {
            const auto button = projectButtonsIter->second;
            m_projectsFlowLayout->removeWidget(button);

            if (!keepProject.contains(projectButtonsIter->first))
            {
                m_fileSystemWatcher->removePath(QDir::toNativeSeparators(button->GetProjectInfo().m_path + "/project.json"));
                button->deleteLater();
                projectButtonsIter = m_projectButtons.erase(projectButtonsIter);
            }
            else
            {
                ++projectButtonsIter;
            }
        }
    }

    void ProjectsScreen::UpdateWithProjects(const QVector<ProjectInfo>& projects)
    {
        PythonBindingsInterface::Get()->RemoveInvalidProjects();

        if (projects.isEmpty() && !m_projectButtons.empty())
        {
            RemoveProjectButtonsFromFlowLayout(projects);
        }

        if (!projects.isEmpty())
        {
            RemoveProjectButtonsFromFlowLayout(projects);

            auto engineInfoResult = PythonBindingsInterface::Get()->GetAllEngineInfos();

            for (const ProjectInfo& project : projects)
            {
                ProjectButton* currentButton = nullptr;
                const AZ::IO::Path projectPath{ project.m_path.toUtf8().constData() };
                auto projectButtonIter = m_projectButtons.find(projectPath);

                EngineInfo engine{};
                if (engineInfoResult && !project.m_enginePath.isEmpty())
                {
                    AZ::IO::FixedMaxPath projectEnginePath{ project.m_enginePath.toUtf8().constData() };
                    for (const EngineInfo& engineInfo : engineInfoResult.GetValue())
                    {
                        AZ::IO::FixedMaxPath enginePath{ engineInfo.m_path.toUtf8().constData() };
                        if (enginePath == projectEnginePath)
                        {
                            engine = engineInfo;
                            break;
                        }
                    }
                }

                if (projectButtonIter == m_projectButtons.end())
                {
                    currentButton = CreateProjectButton(project, engine);
                    m_projectButtons.insert({ projectPath, currentButton });
                    m_fileSystemWatcher->addPath(QDir::toNativeSeparators(project.m_path + "/project.json"));
                }
                else
                {
                    currentButton = projectButtonIter->second;
                    currentButton->SetEngine(engine);
                    currentButton->SetProject(project);
                    currentButton->SetState(ProjectButtonState::ReadyToLaunch);
                }

                AZ_Assert(currentButton, "Invalid ProjectButton");
                m_projectsFlowLayout->addWidget(currentButton);

                bool projectBuiltSuccessfully = false;
                SettingsInterface::Get()->GetProjectBuiltSuccessfully(projectBuiltSuccessfully, project);
                if (!projectBuiltSuccessfully)
                {
                    currentButton->SetState(ProjectButtonState::NeedsToBuild);
                }

                if (project.m_remote)
                {
                    currentButton->SetState(ProjectButtonState::NotDownloaded);
                    currentButton->SetProjectButtonAction(
                        tr("Download Project"),
                        [this, currentButton, project]
                        {
                            m_downloadController->AddObjectDownload(project.m_projectName, "", DownloadController::DownloadObjectType::Project);
                            currentButton->SetState(ProjectButtonState::Downloading);
                        });
                }
            }

            if (m_currentBuilder)
            {
                AZ::IO::Path buildProjectPath = AZ::IO::Path(m_currentBuilder->GetProjectInfo().m_path.toUtf8().constData());
                if (!buildProjectPath.empty())
                {
                    auto buildProjectIter = m_projectButtons.find(buildProjectPath);
                    if (buildProjectIter != m_projectButtons.end())
                    {
                        m_currentBuilder->SetProjectButton(buildProjectIter->second);
                    }
                }
            }

            if (m_currentExporter)
            {
                AZ::IO::Path exportProjectPath = AZ::IO::Path(m_currentExporter->GetProjectInfo().m_path.toUtf8().constData());
                if (!exportProjectPath.empty())
                {
                    if (auto exportProjectIter = m_projectButtons.find(exportProjectPath);
                        exportProjectIter != m_projectButtons.end())
                    {
                        m_currentExporter->SetProjectButton(exportProjectIter->second);
                    }
                }
            }

            for (const ProjectInfo& project : m_buildQueue)
            {
                auto projectIter = m_projectButtons.find(project.m_path.toUtf8().constData());
                if (projectIter != m_projectButtons.end())
                {
                    projectIter->second->SetProjectButtonAction(
                        tr("Cancel queued build"),
                        [this, project]
                        {
                            UnqueueBuildProject(project);
                            SuggestBuildProjectMsg(project, false);
                        });
                }
            }

            for (const ProjectInfo& project : m_exportQueue)
            {
                auto projectIter = m_projectButtons.find(project.m_path.toUtf8().constData());
                if (projectIter != m_projectButtons.end())
                {
                    projectIter->second->SetProjectButtonAction(
                        tr("Cancel queued export"),
                        [this, project]
                        {
                            UnqueueExportProject(project);
                        });
                }
            }

            for (const ProjectInfo& project : m_requiresBuild)
            {
                auto projectIter = m_projectButtons.find(project.m_path.toUtf8().constData());
                if (projectIter != m_projectButtons.end())
                {
                    if (!m_currentBuilder || m_currentBuilder->GetProjectInfo() != project)
                    {
                        if (project.m_buildFailed)
                        {
                            projectIter->second->SetBuildLogsLink(project.m_logUrl);
                            projectIter->second->SetState(ProjectButtonState::BuildFailed);
                        }
                        else
                        {
                            projectIter->second->SetState(ProjectButtonState::NeedsToBuild);
                        }
                    }
                }
            }
        }

        if (m_projectsContent)
        {
            m_stack->setCurrentWidget(m_projectsContent);
        }

        m_projectsFlowLayout->update();

        QTimer::singleShot(0, this,
            [this]
            {
                QPushButton* foundButton = m_stack->currentWidget()->findChild<QPushButton*>();
                if (foundButton)
                {
                    foundButton->setFocus();
                }
            });
    }

    void ProjectsScreen::paintEvent([[maybe_unused]] QPaintEvent* event)
    {
        QPainter painter(this);

        const QSize winSize = size();
        const float pixmapRatio = static_cast<float>(m_background.width()) / m_background.height();
        const float windowRatio = static_cast<float>(winSize.width()) / winSize.height();

        QRect backgroundRect;
        if (pixmapRatio > windowRatio)
        {
            const int newWidth = static_cast<int>(winSize.height() * pixmapRatio);
            const int offset = (newWidth - winSize.width()) / -2;
            backgroundRect = QRect(offset, 0, newWidth, winSize.height());
        }
        else
        {
            const int newHeight = static_cast<int>(winSize.width() / pixmapRatio);
            backgroundRect = QRect(0, 0, winSize.width(), newHeight);
        }

        painter.drawPixmap(backgroundRect, m_background);

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        constexpr float overlayTransparency = 0.3f;
        painter.fillRect(backgroundRect, QColor(0, 0, 0, static_cast<int>(255.0f * overlayTransparency)));
    }

    void ProjectsScreen::HandleNewProjectButton()
    {
        emit ResetScreenRequest(ProjectManagerScreen::CreateProject);
        emit ChangeScreenRequest(ProjectManagerScreen::CreateProject);
    }

    void ProjectsScreen::HandleAddProjectButton()
    {
        QString title{ QObject::tr("Select Project File") };
        QString defaultPath;

        AZ::Outcome<EngineInfo> engineInfoResult = PythonBindingsInterface::Get()->GetEngineInfo();
        if (engineInfoResult.IsSuccess())
        {
            defaultPath = engineInfoResult.GetValue().m_defaultProjectsFolder;
        }

        QString path = QDir::toNativeSeparators(
            QFileDialog::getOpenFileName(this, title, defaultPath, ProjectUtils::ProjectJsonFilename.data()));

        if (!path.isEmpty())
        {
            path.remove(ProjectUtils::ProjectJsonFilename.data());
            if (ProjectUtils::RegisterProject(path, this))
            {
                emit ChangeScreenRequest(ProjectManagerScreen::Projects);
                QMessageBox::information(this, "Project added", "Project added successfully");
            }
        }
    }

    void ProjectsScreen::HandleAddRemoteProjectButton()
    {
        AddRemoteProjectDialog* addRemoteProjectDialog = new AddRemoteProjectDialog(this);
        connect(addRemoteProjectDialog, &AddRemoteProjectDialog::StartObjectDownload, this, &ProjectsScreen::StartProjectDownload);

        if (addRemoteProjectDialog->exec() == QDialog::DialogCode::Accepted)
        {
            QString repoUri = addRemoteProjectDialog->GetRepoPath();
            if (repoUri.isEmpty())
            {
                QMessageBox::warning(this, tr("No Input"), tr("Please provide a repo Uri."));
                return;
            }
        }
    }

    void ProjectsScreen::HandleOpenProject(const QString& projectPath)
    {
        if (projectPath.isEmpty())
        {
            AZ_Error("ProjectManager", false, "Cannot open editor because an empty project path was provided");
            QMessageBox::critical(this, tr("Error"), tr("Failed to launch the Editor because the project path is invalid."));
            return;
        }

        if (WarnIfInBuildQueue(projectPath))
        {
            return;
        }

        AZ::IO::FixedMaxPath fixedProjectPath = projectPath.toUtf8().constData();
        AZ::IO::FixedMaxPath editorExecutablePath = ProjectUtils::GetEditorExecutablePath(fixedProjectPath);

        if (editorExecutablePath.empty())
        {
            AZ_Error("ProjectManager", false, "Failed to locate editor");
            QMessageBox::critical(this, tr("Error"), tr("Failed to locate the Editor, please verify that it is built."));
            return;
        }

        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_commandlineParameters = AZStd::vector<AZStd::string>{
            editorExecutablePath.String(),
            AZStd::string::format(R"(--regset="/Amazon/AzCore/Bootstrap/project_path=%s")", fixedProjectPath.c_str())
        };

        if (!AzFramework::ProcessLauncher::LaunchUnwatchedProcess(processLaunchInfo))
        {
            AZ_Error("ProjectManager", false, "Failed to launch editor");
            QMessageBox::critical(this, tr("Error"), tr("Failed to launch the Editor, please verify the project settings are valid."));
            return;
        }

        ProjectButton* button = qobject_cast<ProjectButton*>(sender());
        if (button)
        {
            button->SetState(ProjectButtonState::Launching);
        }

        constexpr int waitTimeInMs = 3000;
        QTimer::singleShot(waitTimeInMs, this,
            [button]
            {
                if (button)
                {
                    button->SetState(ProjectButtonState::ReadyToLaunch);
                }
            });
    }

    void ProjectsScreen::HandleEditProject(const QString& projectPath)
    {
        if (!WarnIfInBuildQueue(projectPath))
        {
            emit NotifyCurrentProject(projectPath);
            emit ChangeScreenRequest(ProjectManagerScreen::UpdateProject);
        }
    }

    void ProjectsScreen::HandleEditProjectGems(const QString& projectPath)
    {
        if (!WarnIfInBuildQueue(projectPath))
        {
            emit NotifyCurrentProject(projectPath);
            emit ChangeScreenRequest(ProjectManagerScreen::ProjectGemCatalog);
        }
    }

    void ProjectsScreen::HandleCopyProject(const ProjectInfo& projectInfo)
    {
        if (!WarnIfInBuildQueue(projectInfo.m_path))
        {
            ProjectInfo newProjectInfo(projectInfo);

            if (ProjectUtils::CopyProjectDialog(projectInfo.m_path, newProjectInfo, this))
            {
                emit NotifyBuildProject(newProjectInfo);
                emit ChangeScreenRequest(ProjectManagerScreen::Projects);
            }
        }
    }

    void ProjectsScreen::HandleRemoveProject(const QString& projectPath)
    {
        if (!WarnIfInBuildQueue(projectPath))
        {
            if (ProjectUtils::UnregisterProject(projectPath))
            {
                emit ChangeScreenRequest(ProjectManagerScreen::Projects);
                emit NotifyProjectRemoved(projectPath);
            }
        }
    }

    void ProjectsScreen::HandleDeleteProject(const QString& projectPath)
    {
        if (WarnIfInBuildQueue(projectPath))
        {
            return;
        }

        QString projectName = tr("Project");
        auto getProjectResult = PythonBindingsInterface::Get()->GetProject(projectPath);
        if (getProjectResult)
        {
            projectName = getProjectResult.GetValue().m_displayName;
        }

        QMessageBox::StandardButton warningResult = QMessageBox::warning(this,
            tr("Delete %1").arg(projectName),
            tr("%1 will be unregistered from Dusk Engine and the project directory '%2' will be deleted from your disk.\n\nAre you sure you want to delete %1?")
                .arg(projectName, projectPath),
            QMessageBox::No | QMessageBox::Yes);

        if (warningResult == QMessageBox::Yes)
        {
            QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            HandleRemoveProject(projectPath);
            ProjectUtils::DeleteProjectFiles(projectPath);
            QGuiApplication::restoreOverrideCursor();

            emit NotifyProjectRemoved(projectPath);
        }
    }

    void ProjectsScreen::HandleOpenAndroidProjectGenerator(const QString& projectPath)
    {
        AZ::Outcome<EngineInfo> engineInfoResult = PythonBindingsInterface::Get()->GetProjectEngine(projectPath);
        AZ::Outcome projectBuildPathResult = ProjectUtils::GetProjectBuildPath(projectPath);

        auto engineInfo = engineInfoResult.TakeValue();
        auto buildPath = projectBuildPathResult.TakeValue();

        QString projectName = tr("Project");
        auto getProjectResult = PythonBindingsInterface::Get()->GetProject(projectPath);
        if (getProjectResult)
        {
            projectName = getProjectResult.GetValue().m_displayName;
        }

        const QString pythonPath = ProjectUtils::GetPythonExecutablePath(engineInfo.m_path);
        const QString apgPath = QString("%1/Code/Tools/Android/ProjectGenerator/main.py").arg(engineInfo.m_path);

        AZ_Printf("ProjectManager", "APG Info:\nProject Name: %s\nProject Path: %s\nEngine Path: %s\n3rdParty Path: %s\nBuild Path: %s\nPython Path: %s\nAPG path: %s\n",
            projectName.toUtf8().constData(),
            projectPath.toUtf8().constData(),
            engineInfo.m_path.toUtf8().constData(),
            engineInfo.m_thirdPartyPath.toUtf8().constData(),
            buildPath.toUtf8().constData(),
            pythonPath.toUtf8().constData(),
            apgPath.toUtf8().constData());

        QProcess process;
        process.setProgram(pythonPath);
        const QStringList commandArgs{
            apgPath,
            "--e", engineInfo.m_path,
            "--p", projectPath,
            "--b", buildPath,
            "--t", engineInfo.m_thirdPartyPath
        };
        process.setArguments(commandArgs);

        const QString commandArgsStr = QString("%1 %2").arg(pythonPath, commandArgs.join(" "));
        AZ_Printf("ProjectManager", "Will start the Android Project Generator with the following command:\n%s\n",
            commandArgsStr.toUtf8().constData());

        if (!process.startDetached())
        {
            QMessageBox::warning(this, tr("Tool Error"),
                tr("Failed to start Android Project Generator from path %1").arg(apgPath), QMessageBox::Ok);
        }
    }

    void ProjectsScreen::HandleOpenProjectExportSettings(const QString& projectPath)
    {
        AZ::Outcome<EngineInfo> engineInfoResult = PythonBindingsInterface::Get()->GetProjectEngine(projectPath);
        AZ::Outcome projectBuildPathResult = ProjectUtils::GetProjectBuildPath(projectPath);

        auto engineInfo = engineInfoResult.TakeValue();
        auto buildPath = projectBuildPathResult.TakeValue();

        auto getProjectResult = PythonBindingsInterface::Get()->GetProject(projectPath);
        if (!getProjectResult)
        {
            QMessageBox::critical(this, tr("Tool Error"), tr("Failed to retrieve project information."), QMessageBox::Ok);
            return;
        }

        const QString pythonPath = ProjectUtils::GetPythonExecutablePath(engineInfo.m_path);
        const QString o3dePath = QString("%1/scripts/o3de.py").arg(engineInfo.m_path);

        QProcess process;
        process.setProgram(pythonPath);
        const QStringList commandArgs{ o3dePath, "export-project", "-pp", projectPath, "--configure" };
        process.setArguments(commandArgs);

        const QString commandArgsStr = QString("%1 %2").arg(pythonPath, commandArgs.join(" "));
        AZ_Printf("ProjectManager", "Will start the Export Configuration Panel with the following command:\n%s\n",
            commandArgsStr.toUtf8().constData());

        if (!process.startDetached())
        {
            QMessageBox::critical(this, tr("Tool Error"),
                tr("Failed to start o3de.py from path %1").arg(o3dePath), QMessageBox::Ok);
        }
    }

    void ProjectsScreen::HandleProjectFilePathChanged(const QString&)
    {
        UpdateIfCurrentScreen();
    }

    void ProjectsScreen::SuggestBuildProjectMsg(const ProjectInfo& projectInfo, bool showMessage)
    {
        if (RequiresBuildProjectIterator(projectInfo.m_path) == m_requiresBuild.end() || projectInfo.m_buildFailed)
        {
            m_requiresBuild.append(projectInfo);
        }

        UpdateIfCurrentScreen();

        if (showMessage)
        {
            QMessageBox::information(this,
                tr("Project should be rebuilt."),
                projectInfo.GetProjectDisplayName() + tr(" project likely needs to be rebuilt."));
        }
    }

    void ProjectsScreen::SuggestBuildProject(const ProjectInfo& projectInfo)
    {
        SuggestBuildProjectMsg(projectInfo, true);
    }

    void ProjectsScreen::QueueBuildProject(const ProjectInfo& projectInfo, bool skipDialogBox)
    {
        auto requiredIter = RequiresBuildProjectIterator(projectInfo.m_path);
        if (requiredIter != m_requiresBuild.end())
        {
            m_requiresBuild.erase(requiredIter);
        }

        if (BuildQueueContainsProject(projectInfo.m_path))
        {
            return;
        }

        if (m_buildQueue.empty() && !m_currentBuilder)
        {
            StartProjectBuild(projectInfo, skipDialogBox);
        }
        else
        {
            m_buildQueue.append(projectInfo);
            UpdateIfCurrentScreen();
        }
    }

    void ProjectsScreen::UnqueueBuildProject(const ProjectInfo& projectInfo)
    {
        m_buildQueue.removeAll(projectInfo);
        UpdateIfCurrentScreen();
    }

    bool ProjectsScreen::StartProjectBuild(const ProjectInfo& projectInfo, bool skipDialogBox)
    {
        if (!ProjectUtils::FindSupportedCompiler(projectInfo, this))
        {
            return false;
        }

        bool proceedToBuild = skipDialogBox;
        if (!proceedToBuild)
        {
            QMessageBox::StandardButton buildProject = QMessageBox::information(
                this,
                tr("Building \"%1\"").arg(projectInfo.GetProjectDisplayName()),
                tr("Ready to build \"%1\"?").arg(projectInfo.GetProjectDisplayName()),
                QMessageBox::No | QMessageBox::Yes);

            proceedToBuild = buildProject == QMessageBox::Yes;
        }

        if (!proceedToBuild)
        {
            SuggestBuildProjectMsg(projectInfo, false);
            return false;
        }

        m_currentBuilder = new ProjectBuilderController(projectInfo, nullptr, this);
        UpdateWithProjects(GetAllProjects());
        connect(m_currentBuilder, &ProjectBuilderController::Done, this, &ProjectsScreen::ProjectBuildDone);
        connect(m_currentBuilder, &ProjectBuilderController::NotifyBuildProject, this, &ProjectsScreen::SuggestBuildProject);
        m_currentBuilder->Start();

        return true;
    }

    void ProjectsScreen::ProjectBuildDone(bool success)
    {
        ProjectInfo currentBuilderProject;
        if (!success)
        {
            currentBuilderProject = m_currentBuilder->GetProjectInfo();
        }

        delete m_currentBuilder;
        m_currentBuilder = nullptr;

        if (!success)
        {
            SuggestBuildProjectMsg(currentBuilderProject, false);
        }

        if (!m_buildQueue.empty())
        {
            while (!StartProjectBuild(m_buildQueue.front()) && m_buildQueue.size() > 1)
            {
                m_buildQueue.pop_front();
            }
            m_buildQueue.pop_front();
        }

        UpdateIfCurrentScreen();
    }

    void ProjectsScreen::QueueExportProject(const ProjectInfo& projectInfo, const QString& exportScript, bool skipDialogBox)
    {
        if (ExportQueueContainsProject(projectInfo.m_path))
        {
            return;
        }

        if (m_exportQueue.empty() && !m_currentExporter)
        {
            ProjectInfo info = projectInfo;
            info.m_currentExportScript = exportScript;
            StartProjectExport(info, skipDialogBox);
        }
        else
        {
            m_exportQueue.append(projectInfo);
            UpdateIfCurrentScreen();
        }
    }

    void ProjectsScreen::UnqueueExportProject(const ProjectInfo& projectInfo)
    {
        m_exportQueue.removeAll(projectInfo);
        UpdateIfCurrentScreen();
    }

    bool ProjectsScreen::StartProjectExport(const ProjectInfo& projectInfo, bool skipDialogBox)
    {
        bool proceedToExport = skipDialogBox;
        if (!proceedToExport)
        {
            QMessageBox::StandardButton exportProject = QMessageBox::information(
                this,
                tr("Exporting \"%1\"").arg(projectInfo.GetProjectDisplayName()),
                tr("Ready to export \"%1\"? Please ensure you have configured the export settings before proceeding.")
                    .arg(projectInfo.GetProjectDisplayName()),
                QMessageBox::No | QMessageBox::Yes);

            proceedToExport = exportProject == QMessageBox::Yes;
        }

        if (!proceedToExport)
        {
            return false;
        }

        m_currentExporter = AZStd::make_unique<ProjectExportController>(projectInfo, nullptr, this);
        UpdateWithProjects(GetAllProjects());
        connect(m_currentExporter.get(), &ProjectExportController::Done, this, &ProjectsScreen::ProjectExportDone);
        m_currentExporter->Start();

        return true;
    }

    void ProjectsScreen::ProjectExportDone(bool success)
    {
        ProjectInfo currentExportProject;
        if (!success)
        {
            currentExportProject = m_currentExporter->GetProjectInfo();
        }

        m_currentExporter.reset();

        if (!m_exportQueue.empty())
        {
            while (!StartProjectExport(m_exportQueue.front()) && m_exportQueue.size() > 1)
            {
                m_exportQueue.pop_front();
            }
            m_exportQueue.pop_front();
        }

        UpdateIfCurrentScreen();
    }

    void ProjectsScreen::StartProjectDownload(const QString& projectName, const QString& destinationPath, bool queueBuild)
    {
        m_downloadController->AddObjectDownload(projectName, destinationPath, DownloadController::DownloadObjectType::Project);
        UpdateIfCurrentScreen();

        auto foundButton = AZStd::ranges::find_if(m_projectButtons,
            [&projectName](const AZStd::unordered_map<AZ::IO::Path, ProjectButton*>::value_type& value)
            {
                return (value.second->GetProjectInfo().m_projectName == projectName);
            });

        if (foundButton != m_projectButtons.end())
        {
            (*foundButton).second->SetState(queueBuild ? ProjectButtonState::DownloadingBuildQueued : ProjectButtonState::Downloading);
        }
    }

    void ProjectsScreen::HandleDownloadProgress(
        const QString& projectName, DownloadController::DownloadObjectType objectType, int bytesDownloaded, int totalBytes)
    {
        if (objectType != DownloadController::DownloadObjectType::Project)
        {
            return;
        }

        auto foundButton = AZStd::ranges::find_if(m_projectButtons,
            [&projectName](const AZStd::unordered_map<AZ::IO::Path, ProjectButton*>::value_type& value)
            {
                return (value.second->GetProjectInfo().m_projectName == projectName);
            });

        if (foundButton != m_projectButtons.end())
        {
            float percentage = static_cast<float>(bytesDownloaded) / totalBytes;
            (*foundButton).second->SetProgressBarPercentage(percentage);
        }
    }

    void ProjectsScreen::HandleDownloadResult(const QString& projectName, bool succeeded)
    {
        auto foundButton = AZStd::ranges::find_if(m_projectButtons,
            [&projectName](const AZStd::unordered_map<AZ::IO::Path, ProjectButton*>::value_type& value)
            {
                return (value.second->GetProjectInfo().m_projectName == projectName);
            });

        if (foundButton == m_projectButtons.end())
        {
            UpdateIfCurrentScreen();
            return;
        }

        if (!succeeded)
        {
            (*foundButton).second->SetState(ProjectButtonState::NotDownloaded);
            return;
        }

        auto projectsResult = PythonBindingsInterface::Get()->GetProjects();
        if (projectsResult.IsSuccess() && !projectsResult.GetValue().isEmpty())
        {
            for (const ProjectInfo& projectInfo : projectsResult.GetValue())
            {
                if (projectInfo.m_projectName == projectName)
                {
                    (*foundButton).second->SetProject(projectInfo);

                    if ((*foundButton).second->GetState() == ProjectButtonState::DownloadingBuildQueued)
                    {
                        QueueBuildProject(projectInfo, true);
                    }
                    else
                    {
                        (*foundButton).second->SetState(ProjectButtonState::NeedsToBuild);
                    }
                }
            }
        }
    }

    QList<ProjectInfo>::iterator ProjectsScreen::RequiresBuildProjectIterator(const QString& projectPath)
    {
        QString nativeProjPath(QDir::toNativeSeparators(projectPath));
        auto projectIter = m_requiresBuild.begin();
        for (; projectIter != m_requiresBuild.end(); ++projectIter)
        {
            if (QDir::toNativeSeparators(projectIter->m_path) == nativeProjPath)
            {
                break;
            }
        }
        return projectIter;
    }

    bool ProjectsScreen::BuildQueueContainsProject(const QString& projectPath)
    {
        const AZ::IO::PathView path{ projectPath.toUtf8().constData() };
        for (const ProjectInfo& project : m_buildQueue)
        {
            if (AZ::IO::PathView(project.m_path.toUtf8().constData()) == path)
            {
                return true;
            }
        }
        return false;
    }

    bool ProjectsScreen::ExportQueueContainsProject(const QString& projectPath)
    {
        const AZ::IO::PathView path{ projectPath.toUtf8().constData() };
        for (const ProjectInfo& project : m_exportQueue)
        {
            if (AZ::IO::PathView(project.m_path.toUtf8().constData()) == path)
            {
                return true;
            }
        }
        return false;
    }

    bool ProjectsScreen::WarnIfInBuildQueue(const QString& projectPath)
    {
        if (BuildQueueContainsProject(projectPath))
        {
            QMessageBox::warning(this,
                tr("Action Temporarily Disabled!"),
                tr("Action not allowed on projects in build queue."));
            return true;
        }
        return false;
    }
}
