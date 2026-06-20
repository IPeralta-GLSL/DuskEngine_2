#pragma once

#include <QMainWindow>
#include "ImGuiQtOverlay.h"

namespace AzToolsFramework
{

class ImGuiMaterialEditorDockWidget : public QMainWindow
{
    Q_OBJECT
public:
    explicit ImGuiMaterialEditorDockWidget(QWidget* parent = nullptr);
    ~ImGuiMaterialEditorDockWidget();

private:
    ImGuiQtOverlay* m_overlay;
};

} // namespace AzToolsFramework