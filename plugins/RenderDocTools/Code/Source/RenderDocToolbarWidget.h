#pragma once

#include <AzCore/std/string/string.h>
#include <QToolButton>

typedef struct RENDERDOC_API_1_7_0 RENDERDOC_API_1_7_0;

class QMenu;

namespace AZ::Render
{
    class RenderDocToolbarWidget
        : public QToolButton
    {
        Q_OBJECT
    public:
        explicit RenderDocToolbarWidget(RENDERDOC_API_1_7_0* renderdocApi, QWidget* parent = nullptr);

    private:
        void CaptureFrame();
        void OpenLastCapture();
        AZStd::string GetCaptureDirectory() const;

        RENDERDOC_API_1_7_0* m_renderdocApi = nullptr;
        QMenu* m_menu = nullptr;
    };
}
