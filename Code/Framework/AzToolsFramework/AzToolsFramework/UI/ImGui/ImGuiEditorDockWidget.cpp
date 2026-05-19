#include "EditorDefs.h"
#include "ImGuiEditorDockWidget.h"
#include "ImGuiQtOverlay.h"
#include "ImGuiEntityOutliner.h"
#include "ImGuiEntityInspector.h"

#include <QVBoxLayout>

namespace AzToolsFramework
{

ImGuiEditorDockWidget::ImGuiEditorDockWidget(QWidget* parent)
    : QMainWindow(parent)
{
    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);

    m_overlay = new ImGuiQtOverlay(central);
    layout->addWidget(m_overlay);

    setCentralWidget(central);

    ImGuiEntityOutliner::Initialize();
    ImGuiEntityInspector::Initialize();
}

ImGuiEditorDockWidget::~ImGuiEditorDockWidget()
{
    ImGuiEntityOutliner::Shutdown();
    ImGuiEntityInspector::Shutdown();
}

} // namespace AzToolsFramework