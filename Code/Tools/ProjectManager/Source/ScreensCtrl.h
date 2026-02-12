#pragma once

#if !defined(Q_MOC_RUN)
#include <ScreenDefs.h>
#include <ProjectInfo.h>

#include <QStack>
#include <QStackedWidget>
#endif

QT_FORWARD_DECLARE_CLASS(QTabWidget)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(ScreenWidget);
    QT_FORWARD_DECLARE_CLASS(DownloadController);

    class ScreensCtrl : public QWidget
    {
        Q_OBJECT

    public:
        explicit ScreensCtrl(QWidget* parent = nullptr, DownloadController* downloadController = nullptr);
        ~ScreensCtrl() = default;

        void BuildScreens(QVector<ProjectManagerScreen> screens);
        ScreenWidget* FindScreen(ProjectManagerScreen screen);
        ScreenWidget* GetCurrentScreen();

    signals:
        void NotifyCurrentProject(const QString& projectPath);
        void NotifyBuildProject(const ProjectInfo& projectInfo);
        void NotifyProjectRemoved(const QString& projectPath);
        void NotifyRemoteContentRefreshed();

    public slots:
        bool ChangeToScreen(ProjectManagerScreen screen);
        bool ForceChangeToScreen(ProjectManagerScreen screen, bool addVisit = true);
        bool GoToPreviousScreen();
        void ResetScreen(ProjectManagerScreen screen);
        void ResetAllScreens();
        void DeleteScreen(ProjectManagerScreen screen);
        void DeleteAllScreens();
        void TabChanged(int index);

    private:
        int GetScreenTabIndex(ProjectManagerScreen screen);

        QStackedWidget* m_screenStack;
        QTabWidget* m_tabWidget;
        QHash<ProjectManagerScreen, ScreenWidget*> m_screenMap;
        QStack<ProjectManagerScreen> m_screenVisitOrder;
        DownloadController* m_downloadController;
    };
}
