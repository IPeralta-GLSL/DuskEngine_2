#pragma once

#include <QMainWindow>

namespace AzToolsFramework
{

class ImGuiQtOverlay;

class ImGuiEditorDockWidget : public QMainWindow
{
    Q_OBJECT
public:
    explicit ImGuiEditorDockWidget(QWidget* parent = nullptr);
    ~ImGuiEditorDockWidget();

private:
    ImGuiQtOverlay* m_overlay;
};

} // namespace AzToolsFramework