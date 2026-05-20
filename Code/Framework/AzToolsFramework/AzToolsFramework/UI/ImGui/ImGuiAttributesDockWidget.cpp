#include "EditorDefs.h"
#include "ImGuiAttributesDockWidget.h"
#include "ImGuiQtOverlay.h"
#include "ImGuiEntityInspector.h"

#include <QVBoxLayout>

namespace AzToolsFramework
{

ImGuiAttributesDockWidget::ImGuiAttributesDockWidget(QWidget* parent)
    : QMainWindow(parent)
{
    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);

    m_overlay = new ImGuiQtOverlay(ImGuiOverlayContent::Attributes, central);
    layout->addWidget(m_overlay);

    setCentralWidget(central);

    ImGuiEntityInspector::Initialize();
}

ImGuiAttributesDockWidget::~ImGuiAttributesDockWidget()
{
    ImGuiEntityInspector::Shutdown();
}

} // namespace AzToolsFramework