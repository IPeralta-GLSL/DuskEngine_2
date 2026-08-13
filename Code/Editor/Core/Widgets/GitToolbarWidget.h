#pragma once

#include <QToolButton>

class QMenu;
class QTimer;

namespace SandboxEditor
{
    class GitToolbarWidget
        : public QToolButton
    {
        Q_OBJECT
    public:
        explicit GitToolbarWidget(QWidget* parent = nullptr);

        void RefreshStatus();

    private:
        QString ProjectPath() const;
        QString RunGitCapture(const QStringList& args);
        void RunGitCommand(const QString& title, const QStringList& args);
        void ShowCommandResult(const QString& title, const QString& output);
        void UpdateLabel();

        void OnCommit();
        void OnPush();
        void OnPull();
        void OnOpenTerminal();

        QMenu* m_menu = nullptr;
        QTimer* m_refreshTimer = nullptr;
        QString m_branch;
        int m_modifiedCount = 0;
    };
} // namespace SandboxEditor
