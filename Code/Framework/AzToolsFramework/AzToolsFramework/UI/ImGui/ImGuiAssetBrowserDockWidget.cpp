#include "EditorDefs.h"
#include "ImGuiAssetBrowserDockWidget.h"
#include "ImGuiQtOverlay.h"
#include "ImGuiAssetBrowser.h"

#include <QVBoxLayout>

namespace AzToolsFramework
{

ImGuiAssetBrowserDockWidget::ImGuiAssetBrowserDockWidget(QWidget* parent)
    : QMainWindow(parent)
{
    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);

    m_overlay = new ImGuiQtOverlay(ImGuiOverlayContent::AssetBrowser, central);
    layout->addWidget(m_overlay);

    setCentralWidget(central);

    ImGuiAssetBrowser::Initialize();
}

ImGuiAssetBrowserDockWidget::~ImGuiAssetBrowserDockWidget()
{
    ImGuiAssetBrowser::Shutdown();
}

} // namespace AzToolsFramework