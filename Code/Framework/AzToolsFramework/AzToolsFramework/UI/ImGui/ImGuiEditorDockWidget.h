#pragma once

#include <QMainWindow>
#include "ImGuiQtOverlay.h"

namespace AzToolsFramework
{

class ImGuiEditorDockWidget : public QMainWindow
{
    Q_OBJECT
public:
    explicit ImGuiEditorDockWidget(ImGuiOverlayContent contentMode, QWidget* parent = nullptr);
    ~ImGuiEditorDockWidget();

    ImGuiOverlayContent GetContentMode() const { return m_contentMode; }

private:
    ImGuiOverlayContent m_contentMode;
    ImGuiQtOverlay* m_overlay;
};

} // namespace AzToolsFramework