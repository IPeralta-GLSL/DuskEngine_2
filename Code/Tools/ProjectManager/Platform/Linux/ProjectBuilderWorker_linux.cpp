#include <ProjectBuilderWorker.h>
#include <ProjectManagerDefs.h>
#include <ProjectUtils.h>

#include <QDir>
#include <QFileInfo>
#include <QString>

namespace O3DE::ProjectManager
{
    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeGenerateProjectArguments(const EngineInfo& engineInfo) const
    {
        QString engineBuildPath = QDir(engineInfo.m_path).filePath(ProjectBuildPathPostfix);
        bool    existingBuild = QFileInfo::exists(QDir(engineBuildPath).filePath("CMakeCache.txt"));

        QStringList generateProjectArgs = QStringList{ProjectCMakeCommand,
                                                      "-B", engineBuildPath,
                                                      "-S", engineInfo.m_path,
                                                      QString("-DLY_3RDPARTY_PATH=").append(engineInfo.m_thirdPartyPath),
                                                      QString("-DLY_PROJECTS=").append(m_projectInfo.m_path)};

        if (!existingBuild)
        {
            auto    whichNinjaResult = ProjectUtils::ExecuteCommandResult("which", QStringList{"ninja"});
            if (whichNinjaResult.IsSuccess())
            {
                generateProjectArgs.append("-GNinja");
            }
            generateProjectArgs.append("-DCMAKE_BUILD_TYPE=profile");
        }

        return AZ::Success(generateProjectArgs);
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeBuildCommandArguments(const EngineInfo& engineInfo) const
    {
        QString engineBuildPath = QDir(engineInfo.m_path).filePath(ProjectBuildPathPostfix);
        const QString gameLauncherTargetName = m_projectInfo.m_projectName + ".GameLauncher";
        const QString headlessServerLauncherTargetName = m_projectInfo.m_projectName + ".HeadlessServerLauncher";
        const QString serverLauncherTargetName = m_projectInfo.m_projectName + ".ServerLauncher";
        const QString unifiedLauncherTargetName = m_projectInfo.m_projectName + ".UnifiedLauncher";

        QStringList buildProjectArgs = QStringList{ProjectCMakeCommand,
                                                   "--build", engineBuildPath,
                                                   "--target", gameLauncherTargetName, headlessServerLauncherTargetName, serverLauncherTargetName, unifiedLauncherTargetName, ProjectCMakeBuildTargetEditor};
        return AZ::Success(buildProjectArgs);
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructKillProcessCommandArguments(const QString& pidToKill) const
    {
        return AZ::Success(QStringList{"kill", "-9", pidToKill});
    }

} // namespace O3DE::ProjectManager
