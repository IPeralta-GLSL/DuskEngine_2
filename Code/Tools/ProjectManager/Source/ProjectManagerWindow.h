#pragma once

#if !defined(Q_MOC_RUN)
#include <QMainWindow>
#include <QPointer>
#include <AzCore/IO/Path/Path.h>
#include <ScreenDefs.h>
#endif

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(DownloadController);

    class ProjectManagerWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit ProjectManagerWindow(
            QWidget* parent,
            const AZ::IO::PathView& projectPath,
            ProjectManagerScreen startScreen = ProjectManagerScreen::Projects);

    private:
        void closeEvent(QCloseEvent* event) override;

        QPointer<DownloadController> m_downloadController;
    };
}
