/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectBuilderWorker.h>
#include <ProjectManagerDefs.h>
#include <ProjectUtils.h>

#include <QDir>
#include <QString>

namespace O3DE::ProjectManager
{
    namespace Internal
    {
        AZ::Outcome<QString, QString> QueryInstalledCmakeFullPath()
        {
            auto environmentRequest = ProjectUtils::SetupCommandLineProcessEnvironment();
            if (!environmentRequest.IsSuccess())
            {
                return AZ::Failure(environmentRequest.GetError());
            }

            auto queryCmakeInstalled = ProjectUtils::ExecuteCommandResult("which",
                                                                          QStringList{ProjectCMakeCommand});
            if (!queryCmakeInstalled.IsSuccess())
            {
                return AZ::Failure(QObject::tr("Unable to detect CMake on this host."));
            }
            QString cmakeInstalledPath = queryCmakeInstalled.GetValue().split("\n")[0];
            return AZ::Success(cmakeInstalledPath);
        }
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeGenerateProjectArguments(const EngineInfo& engineInfo) const
    {
        auto cmakeInstalledPathQuery = Internal::QueryInstalledCmakeFullPath();
        if (!cmakeInstalledPathQuery.IsSuccess())
        {
            return AZ::Failure(cmakeInstalledPathQuery.GetError());
        }
        QString cmakeInstalledPath = cmakeInstalledPathQuery.GetValue();
        QString engineBuildPath = QDir(engineInfo.m_path).filePath(ProjectBuildPathPostfix);

        return AZ::Success(QStringList{cmakeInstalledPath,
                                       "-B", engineBuildPath,
                                       "-S", engineInfo.m_path,
                                       "-GXcode",
                                       QString("-DLY_PROJECTS=").append(m_projectInfo.m_path)});
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructCmakeBuildCommandArguments(const EngineInfo& engineInfo) const
    {
        auto cmakeInstalledPathQuery = Internal::QueryInstalledCmakeFullPath();
        if (!cmakeInstalledPathQuery.IsSuccess())
        {
            return AZ::Failure(cmakeInstalledPathQuery.GetError());
        }

        QString cmakeInstalledPath = cmakeInstalledPathQuery.GetValue();
        QString engineBuildPath = QDir(engineInfo.m_path).filePath(ProjectBuildPathPostfix);
        QString launcherTargetName = m_projectInfo.m_projectName + ".GameLauncher";

        return AZ::Success(QStringList{cmakeInstalledPath,
                                        "--build", engineBuildPath,
                                        "--config", "profile",
                                        "--target", launcherTargetName, ProjectCMakeBuildTargetEditor});
    }

    AZ::Outcome<QStringList, QString> ProjectBuilderWorker::ConstructKillProcessCommandArguments(const QString& pidToKill) const
    {
        return AZ::Success(QStringList{"kill", "-9", pidToKill});
    }
    
} // namespace O3DE::ProjectManager
