#include "EditorDefs.h"
#include "ImGuiStageDockWidget.h"
#include "ImGuiQtOverlay.h"
#include "ImGuiEntityOutliner.h"

#include <QVBoxLayout>

namespace AzToolsFramework
{

ImGuiStageDockWidget::ImGuiStageDockWidget(QWidget* parent)
    : QMainWindow(parent)
{
    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);

    m_overlay = new ImGuiQtOverlay(ImGuiOverlayContent::Stage, central);
    layout->addWidget(m_overlay);

    setCentralWidget(central);

    ImGuiEntityOutliner::Initialize();
}

ImGuiStageDockWidget::~ImGuiStageDockWidget()
{
    ImGuiEntityOutliner::Shutdown();
}

} // namespace AzToolsFramework