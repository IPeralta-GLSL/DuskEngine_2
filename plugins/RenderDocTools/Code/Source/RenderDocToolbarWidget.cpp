#include "RenderDocToolbarWidget.h"

#include <renderdoc_app.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>

#include <QDir>
#include <QFileInfoList>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>

namespace AZ::Render
{
    namespace
    {
        constexpr char ToolbarLogName[] = "RenderDocTools";
        constexpr char CaptureDirName[] = "renderdoccaptures";
        constexpr char CaptureFileTemplate[] = "DuskEngine_capture";
    }

    RenderDocToolbarWidget::RenderDocToolbarWidget(RENDERDOC_API_1_7_0* renderdocApi, QWidget* parent)
        : QToolButton(parent)
        , m_renderdocApi(renderdocApi)
    {
        setPopupMode(QToolButton::InstantPopup);
        setToolButtonStyle(Qt::ToolButtonIconOnly);
        setIcon(QIcon(QStringLiteral(":/RenderDocTools/renderdoc_logo.png")));
        setToolTip("RenderDoc frame captures");

        m_menu = new QMenu(this);
        m_menu->addAction(QStringLiteral("Capture current frame"), this, [this]()
        {
            CaptureFrame();
        });
        m_menu->addAction(QStringLiteral("Open last capture in RenderDoc"), this, [this]()
        {
            OpenLastCapture();
        });
        setMenu(m_menu);
    }

    AZStd::string RenderDocToolbarWidget::GetCaptureDirectory() const
    {
        AZStd::string captureDir = CaptureDirName;
        const AZ::IO::FileIOBase* fileIo = AZ::IO::FileIOBase::GetInstance();
        if (fileIo)
        {
            char projectPath[AZ_MAX_PATH_LEN] = { 0 };
            if (fileIo->ResolvePath("@projectpath@", projectPath, AZ_MAX_PATH_LEN))
            {
                captureDir = projectPath;
                captureDir += "/";
                captureDir += CaptureDirName;
            }
        }
        return captureDir;
    }

    void RenderDocToolbarWidget::CaptureFrame()
    {
        if (!m_renderdocApi)
        {
            AZ_Warning(ToolbarLogName, false, "Capture requested but the RenderDoc in-app API is not loaded.");
            QMessageBox::warning(
                this,
                QStringLiteral("RenderDoc"),
                QStringLiteral(
                    "La API de RenderDoc no está cargada en esta sesión.\n"
                    "Verificá que RenderDoc esté instalado (librenderdoc.so) y reiniciá el Editor."));
            return;
        }

        AZStd::string captureDir = GetCaptureDirectory();
        QDir().mkpath(QString(captureDir.c_str()));

        AZStd::string fileTemplate = captureDir;
        fileTemplate += "/";
        fileTemplate += CaptureFileTemplate;
        m_renderdocApi->SetCaptureFilePathTemplate(fileTemplate.c_str());

        m_renderdocApi->StartFrameCapture(nullptr, nullptr);
        m_renderdocApi->EndFrameCapture(nullptr, nullptr);

        AZ_Printf(ToolbarLogName, "Frame capture requested. Captures are saved under: %s", captureDir.c_str());
        QMessageBox::information(
            this,
            QStringLiteral("RenderDoc"),
            QStringLiteral("Captura del próximo frame solicitada.\nSe guardará en:\n%1").arg(QString(captureDir.c_str())));
    }

    void RenderDocToolbarWidget::OpenLastCapture()
    {
        AZStd::string captureDir = GetCaptureDirectory();
        QDir dir(QString(captureDir.c_str()));
        QFileInfoList captures = dir.entryInfoList(QStringList() << QStringLiteral("*.rdc"), QDir::Files, QDir::Time);

        if (captures.isEmpty())
        {
            QMessageBox::information(
                this,
                QStringLiteral("RenderDoc"),
                QStringLiteral("No hay capturas todavía en:\n%1\n\nUsá \"Capture current frame\" primero.")
                    .arg(QString(captureDir.c_str())));
            return;
        }

        const QString capturePath = captures.first().absoluteFilePath();

        bool launched = false;
        if (m_renderdocApi && m_renderdocApi->LaunchReplayUI)
        {
            const QByteArray pathUtf8 = capturePath.toUtf8();
            launched = (m_renderdocApi->LaunchReplayUI(0, pathUtf8.constData()) != 0);
        }
        if (!launched)
        {
            launched = QProcess::startDetached(QStringLiteral("qrenderdoc"), QStringList() << capturePath);
        }

        if (launched)
        {
            AZ_Printf(ToolbarLogName, "Opening capture in RenderDoc: %s", capturePath.toUtf8().constData());
        }
        else
        {
            AZ_Warning(ToolbarLogName, false, "Failed to launch qrenderdoc for capture: %s", capturePath.toUtf8().constData());
            QMessageBox::warning(
                this,
                QStringLiteral("RenderDoc"),
                QStringLiteral("No se pudo abrir qrenderdoc.\nCaptura: %1").arg(capturePath));
        }
    }
}
