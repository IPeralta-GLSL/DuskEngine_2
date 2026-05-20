#pragma once

#include <QMainWindow>
#include "ImGuiQtOverlay.h"

namespace AzToolsFramework
{

class ImGuiAttributesDockWidget : public QMainWindow
{
    Q_OBJECT
public:
    explicit ImGuiAttributesDockWidget(QWidget* parent = nullptr);
    ~ImGuiAttributesDockWidget();

private:
    ImGuiQtOverlay* m_overlay;
};

} // namespace AzToolsFramework