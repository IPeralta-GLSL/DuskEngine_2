#pragma once

#include <QMainWindow>
#include "ImGuiQtOverlay.h"

namespace AzToolsFramework
{

class ImGuiStageDockWidget : public QMainWindow
{
    Q_OBJECT
public:
    explicit ImGuiStageDockWidget(QWidget* parent = nullptr);
    ~ImGuiStageDockWidget();

private:
    ImGuiQtOverlay* m_overlay;
};

} // namespace AzToolsFramework