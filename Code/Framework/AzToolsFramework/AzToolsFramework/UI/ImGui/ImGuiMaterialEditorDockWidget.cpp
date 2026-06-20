#include "EditorDefs.h"
#include "ImGuiMaterialEditorDockWidget.h"
#include "ImGuiQtOverlay.h"
#include "ImGuiMaterialNodeEditor.h"

#include <QVBoxLayout>

namespace AzToolsFramework
{

ImGuiMaterialEditorDockWidget::ImGuiMaterialEditorDockWidget(QWidget* parent)
    : QMainWindow(parent)
{
    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);

    m_overlay = new ImGuiQtOverlay(ImGuiOverlayContent::MaterialEditor, central);
    layout->addWidget(m_overlay);

    setCentralWidget(central);

    ImGuiMaterialNodeEditor::Initialize();
}

ImGuiMaterialEditorDockWidget::~ImGuiMaterialEditorDockWidget()
{
    ImGuiMaterialNodeEditor::Shutdown();
}

} // namespace AzToolsFramework