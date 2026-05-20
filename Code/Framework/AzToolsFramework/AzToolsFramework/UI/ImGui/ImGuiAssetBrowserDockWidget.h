#pragma once

#include <QMainWindow>
#include "ImGuiQtOverlay.h"

namespace AzToolsFramework
{

class ImGuiAssetBrowserDockWidget : public QMainWindow
{
    Q_OBJECT
public:
    explicit ImGuiAssetBrowserDockWidget(QWidget* parent = nullptr);
    ~ImGuiAssetBrowserDockWidget();

private:
    ImGuiQtOverlay* m_overlay;
};

} // namespace AzToolsFramework