#include "ProjectManagerWindow.h"
#include "DownloadController.h"
#include "ProjectManagerBuses.h"
#include "PythonBindingsInterface.h"
#include "ScreensCtrl.h"

#include <QCloseEvent>
#include <QMessageBox>

namespace O3DE::ProjectManager
{
    ProjectManagerWindow::ProjectManagerWindow(QWidget* parent, const AZ::IO::PathView& projectPath, ProjectManagerScreen startScreen)
        : QMainWindow(parent)
    {
        if (auto engineInfoOutcome = PythonBindingsInterface::Get()->GetEngineInfo(); engineInfoOutcome)
        {
            auto engineInfo = engineInfoOutcome.GetValue<EngineInfo>();
            auto versionToDisplay = engineInfo.m_displayVersion == "00.00"
                ? engineInfo.m_version
                : engineInfo.m_displayVersion;
            setWindowTitle(QString("%1 %2 %3").arg(engineInfo.m_name.toUpper(), versionToDisplay, tr("Project Manager")));
        }
        else
        {
            setWindowTitle(QString("Dusk Engine %1").arg(tr("Project Manager")));
        }

        m_downloadController = new DownloadController(this);

        ScreensCtrl* screensCtrl = new ScreensCtrl(nullptr, m_downloadController);

        QVector<ProjectManagerScreen> screenEnums =
        {
            ProjectManagerScreen::Projects,
            ProjectManagerScreen::CreateGem,
            ProjectManagerScreen::EditGem,
            ProjectManagerScreen::GemCatalog,
            ProjectManagerScreen::Engine,
            ProjectManagerScreen::CreateProject,
            ProjectManagerScreen::UpdateProject,
            ProjectManagerScreen::GemsGemRepos
        };
        screensCtrl->BuildScreens(screenEnums);
        setCentralWidget(screensCtrl);

        if (startScreen != ProjectManagerScreen::Projects)
        {
            screensCtrl->ForceChangeToScreen(ProjectManagerScreen::Projects);
            screensCtrl->ForceChangeToScreen(startScreen);
        }

        if (!projectPath.empty())
        {
            const QString path = QString::fromUtf8(projectPath.Native().data(), aznumeric_cast<int>(projectPath.Native().size()));
            emit screensCtrl->NotifyCurrentProject(path);
        }
    }

    void ProjectManagerWindow::closeEvent(QCloseEvent* event)
    {
        bool canClose = true;
        ProjectManagerUtilityRequestsBus::Broadcast(&ProjectManagerUtilityRequestsBus::Events::CanCloseProjectManager, canClose);

        if (!canClose)
        {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                tr("Project action ongoing"),
                tr("A project action is currently going on. Are you sure you want to exit?"),
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::No)
            {
                event->ignore();
                return;
            }
        }

        QMainWindow::closeEvent(event);
    }
}
