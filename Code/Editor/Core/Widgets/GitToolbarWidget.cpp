/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Core/Widgets/GitToolbarWidget.h>

#include <AzCore/Utils/Utils.h>

#include <QAction>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>

namespace SandboxEditor
{
    GitToolbarWidget::GitToolbarWidget(QWidget* parent)
        : QToolButton(parent)
    {
        setPopupMode(QToolButton::InstantPopup);
        setToolButtonStyle(Qt::ToolButtonIconOnly);
        setIcon(QIcon(":/stylesheet/img/UI20/toolbar/git.svg"));
        setToolTip("Git version control");

        m_menu = new QMenu(this);
        m_statusAction = m_menu->addAction("(loading...)");
        m_statusAction->setEnabled(false);
        m_menu->addSeparator();
        m_menu->addAction("Refresh Status", this, &GitToolbarWidget::RefreshStatus);
        m_menu->addSeparator();
        m_menu->addAction("Commit...", this, &GitToolbarWidget::OnCommit);
        m_menu->addAction("Push", this, &GitToolbarWidget::OnPush);
        m_menu->addAction("Pull", this, &GitToolbarWidget::OnPull);
        m_menu->addSeparator();
        m_menu->addAction("Open Git Terminal", this, &GitToolbarWidget::OnOpenTerminal);
        setMenu(m_menu);

        m_refreshTimer = new QTimer(this);
        m_refreshTimer->setInterval(5000);
        connect(m_refreshTimer, &QTimer::timeout, this, &GitToolbarWidget::RefreshStatus);
        m_refreshTimer->start();

        RefreshStatus();
    }

    QString GitToolbarWidget::ProjectPath() const
    {
        return QString::fromUtf8(AZ::Utils::GetProjectPath().c_str());
    }

    QString GitToolbarWidget::RunGitCapture(const QStringList& args)
    {
        QProcess process;
        process.setWorkingDirectory(ProjectPath());
        process.start("git", args);
        if (!process.waitForFinished(15000))
        {
            return {};
        }
        return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    }

    void GitToolbarWidget::RunGitCommand(const QString& title, const QStringList& args)
    {
        QProcess process;
        process.setWorkingDirectory(ProjectPath());
        process.start("git", args);
        if (!process.waitForFinished(30000))
        {
            ShowCommandResult(title, "Command timed out");
            return;
        }
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        QString error = QString::fromUtf8(process.readAllStandardError());
        if (!error.isEmpty())
        {
            output += error;
        }
        ShowCommandResult(title, output.trimmed());
        RefreshStatus();
    }

    void GitToolbarWidget::ShowCommandResult(const QString& title, const QString& output)
    {
        QMessageBox::information(this, title, output.isEmpty() ? "(no output)" : output);
    }

    void GitToolbarWidget::UpdateLabel()
    {
        if (m_modifiedCount > 0)
        {
            m_statusAction->setText(QString("Git: %1 (%2)").arg(m_branch).arg(m_modifiedCount));
        }
        else
        {
            m_statusAction->setText(QString("Git: %1").arg(m_branch));
        }
        setToolTip(QString("%1 - %2 modified file(s)").arg(m_branch).arg(m_modifiedCount));
    }

    void GitToolbarWidget::RefreshStatus()
    {
        m_branch = RunGitCapture({"rev-parse", "--abbrev-ref", "HEAD"});
        if (m_branch.isEmpty())
        {
            m_branch = "(no repo)";
            m_modifiedCount = 0;
        }
        else
        {
            QString status = RunGitCapture({"status", "--porcelain"});
            m_modifiedCount = status.isEmpty() ? 0 : status.count('\n') + 1;
        }
        UpdateLabel();
    }

    void GitToolbarWidget::OnCommit()
    {
        bool ok = false;
        QString message = QInputDialog::getText(this, "Git Commit", "Commit message:", QLineEdit::Normal, {}, &ok);
        if (!ok || message.isEmpty())
        {
            return;
        }
        RunGitCommand("Git Commit", {"add", "-A"});
        RunGitCommand("Git Commit", {"commit", "-m", message});
    }

    void GitToolbarWidget::OnPush()
    {
        RunGitCommand("Git Push", {"push"});
    }

    void GitToolbarWidget::OnPull()
    {
        RunGitCommand("Git Pull", {"pull"});
    }

    void GitToolbarWidget::OnOpenTerminal()
    {
        QString path = ProjectPath();
        if (QProcess::startDetached("xterm", {"-e", "bash", "-c", QString("git -C '%1' status; exec bash").arg(path)}))
        {
            return;
        }
        if (QProcess::startDetached("konsole", {"--workdir", path, "-e", "bash"}))
        {
            return;
        }
        QProcess::startDetached("gnome-terminal", {"--working-directory", path});
    }
} // namespace SandboxEditor
