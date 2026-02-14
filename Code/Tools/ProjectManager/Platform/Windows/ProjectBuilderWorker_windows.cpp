#include <ProjectBuilderWorker.h>
#include <ProjectManagerDefs.h>

#include <QDir>
#include <QString>

namespace O3DE::ProjectManager
{
    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeGenerateProjectArguments(const EngineInfo& engineInfo) const
    {
        QString engineBuildPath = QDir(engineInfo.m_path).filePath(ProjectBuildPathPostfix);

        QStringList args = { 
            ProjectCMakeCommand,
            "-B", engineBuildPath,
            "-S", engineInfo.m_path,
            QString("-DLY_3RDPARTY_PATH=").append(engineInfo.m_thirdPartyPath),
            QString("-DLY_PROJECTS=").append(m_projectInfo.m_path)
            };

        if (m_projectInfo.m_isScriptOnly)
        {
            args.append("-GNinja Multi-Config");
        }

        return AZ::Success( args );
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeBuildCommandArguments(const EngineInfo& engineInfo) const
    {
        const QString engineBuildPath = QDir(engineInfo.m_path).filePath(ProjectBuildPathPostfix);
        const QString gameLauncherTargetName = m_projectInfo.m_projectName + ".GameLauncher";
        const QString headlessServerLauncherTargetName = m_projectInfo.m_projectName + ".HeadlessServerLauncher";
        const QString serverLauncherTargetName = m_projectInfo.m_projectName + ".ServerLauncher";
        const QString unifiedLauncherTargetName = m_projectInfo.m_projectName + ".UnifiedLauncher";

        return AZ::Success(QStringList{ ProjectCMakeCommand,
                                        "--build", engineBuildPath,
                                        "--config", "profile",
                                        "--target", gameLauncherTargetName, headlessServerLauncherTargetName, serverLauncherTargetName, unifiedLauncherTargetName, ProjectCMakeBuildTargetEditor });
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructKillProcessCommandArguments(const QString& pidToKill) const
    {
        return AZ::Success(QStringList { "cmd.exe", "/C", "taskkill", "/pid", pidToKill, "/f", "/t" } );
    }

} // namespace O3DE::ProjectManager
