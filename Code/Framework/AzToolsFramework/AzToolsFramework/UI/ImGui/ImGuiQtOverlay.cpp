#include "EditorDefs.h"
#include "ImGuiQtOverlay.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

#include <imgui/imgui.h>
#include "imgui_impl_opengl3.h"

#include "ImGuiTheme.h"

#include "ImGuiEntityOutliner.h"
#include "ImGuiEntityInspector.h"
#include "ImGuiAssetBrowser.h"
#include "ImGuiMaterialNodeEditor.h"

namespace AzToolsFramework
{

ImGuiQtOverlay::ImGuiQtOverlay(ImGuiOverlayContent contentMode, QWidget* parent)
    : QOpenGLWidget(parent)
    , m_imguiContext(nullptr)
    , m_contentMode(contentMode)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, [this]() { update(); });
    m_renderTimer->start(16);
}

ImGuiQtOverlay::~ImGuiQtOverlay()
{
    m_renderTimer->stop();
    ShutdownImGui();
}


void ImGuiQtOverlay::initializeGL()
{
    SetupImGui();
}

void ImGuiQtOverlay::SetupImGui()
{
    IMGUI_CHECKVERSION();
    m_imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_imguiContext);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    // Apply the shared dark theme (ImGuiTheme.h mirrors the Qt stylesheet palette)
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    using namespace ImGuiTheme;
    ImVec4* colors = style.Colors;

    // Background colors
    colors[ImGuiCol_WindowBg] = WindowBg;
    colors[ImGuiCol_ChildBg] = PanelBg;
    colors[ImGuiCol_PopupBg] = PanelBg;
    colors[ImGuiCol_FrameBg] = InputBg;
    colors[ImGuiCol_FrameBgHovered] = HoverBg;
    colors[ImGuiCol_FrameBgActive] = HoverBg;

    // Text colors
    colors[ImGuiCol_Text] = TextNormal;
    colors[ImGuiCol_TextDisabled] = TextDisabled;

    // Headers and titles
    colors[ImGuiCol_Header] = HeaderBg;
    colors[ImGuiCol_HeaderHovered] = HoverBg;
    colors[ImGuiCol_HeaderActive] = Accent;

    // Buttons
    colors[ImGuiCol_Button] = InputBg;
    colors[ImGuiCol_ButtonHovered] = HoverBg;
    colors[ImGuiCol_ButtonActive] = Accent;

    // Title bar
    colors[ImGuiCol_TitleBg] = PanelBg;
    colors[ImGuiCol_TitleBgActive] = HeaderBg;
    colors[ImGuiCol_TitleBgCollapsed] = WindowBg;

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = WindowBg;
    colors[ImGuiCol_ScrollbarGrab] = Handle;
    colors[ImGuiCol_ScrollbarGrabHovered] = Border;
    colors[ImGuiCol_ScrollbarGrabActive] = Accent;

    // Separator and borders
    colors[ImGuiCol_Separator] = Border;
    colors[ImGuiCol_SeparatorHovered] = AccentMuted;
    colors[ImGuiCol_SeparatorActive] = Accent;

    // Selection and highlights
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
    colors[ImGuiCol_NavHighlight] = AccentMuted;

    // Check mark
    colors[ImGuiCol_CheckMark] = Accent;

    // Slider
    colors[ImGuiCol_SliderGrab] = AccentMuted;
    colors[ImGuiCol_SliderGrabActive] = Accent;

    ImGui_ImplOpenGL3_Init("#version 330");
}

void ImGuiQtOverlay::paintGL()
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    RenderImGuiContent();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiQtOverlay::RenderImGuiContent()
{
    switch (m_contentMode)
    {
    case ImGuiOverlayContent::Stage:
        ImGuiEntityOutliner::Render();
        break;
    case ImGuiOverlayContent::Attributes:
        ImGuiEntityInspector::Render();
        break;
    case ImGuiOverlayContent::AssetBrowser:
        ImGuiAssetBrowser::Render();
        break;
    case ImGuiOverlayContent::MaterialEditor:
        ImGuiMaterialNodeEditor::Render();
        break;
    }
}

void ImGuiQtOverlay::resizeGL(int w, int h)
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
}

void ImGuiQtOverlay::ShutdownImGui()
{
    if (m_imguiContext)
    {
        ImGui::SetCurrentContext(m_imguiContext);
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext(m_imguiContext);
        m_imguiContext = nullptr;
    }
}

void ImGuiQtOverlay::mousePressEvent(QMouseEvent* event)
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(static_cast<float>(event->pos().x()), static_cast<float>(event->pos().y()));

    if (event->button() == Qt::LeftButton) io.MouseDown[0] = true;
    if (event->button() == Qt::RightButton) io.MouseDown[1] = true;
    if (event->button() == Qt::MiddleButton) io.MouseDown[2] = true;

    if (io.WantCaptureMouse)
    {
        event->accept();
        return;
    }
    QOpenGLWidget::mousePressEvent(event);
}

void ImGuiQtOverlay::mouseReleaseEvent(QMouseEvent* event)
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGuiIO& io = ImGui::GetIO();

    if (event->button() == Qt::LeftButton) io.MouseDown[0] = false;
    if (event->button() == Qt::RightButton) io.MouseDown[1] = false;
    if (event->button() == Qt::MiddleButton) io.MouseDown[2] = false;

    if (io.WantCaptureMouse)
    {
        event->accept();
        return;
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void ImGuiQtOverlay::mouseMoveEvent(QMouseEvent* event)
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(static_cast<float>(event->pos().x()), static_cast<float>(event->pos().y()));

    if (io.WantCaptureMouse)
    {
        event->accept();
        return;
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void ImGuiQtOverlay::wheelEvent(QWheelEvent* event)
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel += event->angleDelta().y() / 120.0f;

    if (io.WantCaptureMouse)
    {
        event->accept();
        return;
    }
    QOpenGLWidget::wheelEvent(event);
}

void ImGuiQtOverlay::keyPressEvent(QKeyEvent* event)
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGuiIO& io = ImGui::GetIO();

    // Teclas de modificadores
    io.KeyCtrl = (event->modifiers() & Qt::ControlModifier) != 0;
    io.KeyShift = (event->modifiers() & Qt::ShiftModifier) != 0;
    io.KeyAlt = (event->modifiers() & Qt::AltModifier) != 0;

    // En ImGui v1.82, las teclas especiales tienen valores específicos
    int key = event->key();
    if (key >= Qt::Key_A && key <= Qt::Key_Z)
        io.KeysDown[ImGuiKey_A + (key - Qt::Key_A)] = true;
    else if (key == Qt::Key_Return || key == Qt::Key_Enter)
        io.KeysDown[ImGuiKey_Enter] = true;
    else if (key == Qt::Key_Escape)
        io.KeysDown[ImGuiKey_Escape] = true;
    else if (key == Qt::Key_Tab)
        io.KeysDown[ImGuiKey_Tab] = true;
    else if (key == Qt::Key_Backspace)
        io.KeysDown[ImGuiKey_Backspace] = true;
    else if (key == Qt::Key_Delete)
        io.KeysDown[ImGuiKey_Delete] = true;
    else if (key == Qt::Key_Up)
        io.KeysDown[ImGuiKey_UpArrow] = true;
    else if (key == Qt::Key_Down)
        io.KeysDown[ImGuiKey_DownArrow] = true;
    else if (key == Qt::Key_Left)
        io.KeysDown[ImGuiKey_LeftArrow] = true;
    else if (key == Qt::Key_Right)
        io.KeysDown[ImGuiKey_RightArrow] = true;
    else if (key == Qt::Key_Space)
        io.KeysDown[ImGuiKey_Space] = true;

    // Caracteres de texto
    if (!event->text().isEmpty())
    {
        io.AddInputCharactersUTF8(event->text().toUtf8().constData());
    }

    if (io.WantCaptureKeyboard)
    {
        event->accept();
        return;
    }
    QOpenGLWidget::keyPressEvent(event);
}

void ImGuiQtOverlay::keyReleaseEvent(QKeyEvent* event)
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGuiIO& io = ImGui::GetIO();

    int key = event->key();
    if (key >= Qt::Key_A && key <= Qt::Key_Z)
        io.KeysDown[ImGuiKey_A + (key - Qt::Key_A)] = false;
    else if (key == Qt::Key_Return || key == Qt::Key_Enter)
        io.KeysDown[ImGuiKey_Enter] = false;
    else if (key == Qt::Key_Escape)
        io.KeysDown[ImGuiKey_Escape] = false;
    else if (key == Qt::Key_Tab)
        io.KeysDown[ImGuiKey_Tab] = false;
    else if (key == Qt::Key_Backspace)
        io.KeysDown[ImGuiKey_Backspace] = false;
    else if (key == Qt::Key_Delete)
        io.KeysDown[ImGuiKey_Delete] = false;
    else if (key == Qt::Key_Up)
        io.KeysDown[ImGuiKey_UpArrow] = false;
    else if (key == Qt::Key_Down)
        io.KeysDown[ImGuiKey_DownArrow] = false;
    else if (key == Qt::Key_Left)
        io.KeysDown[ImGuiKey_LeftArrow] = false;
    else if (key == Qt::Key_Right)
        io.KeysDown[ImGuiKey_RightArrow] = false;
    else if (key == Qt::Key_Space)
        io.KeysDown[ImGuiKey_Space] = false;

    if (io.WantCaptureKeyboard)
    {
        event->accept();
        return;
    }
    QOpenGLWidget::keyReleaseEvent(event);
}

} // namespace AzToolsFramework