#include "EditorDefs.h"
#include "ImGuiQtOverlay.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

#include <imgui/imgui.h>
#include "imgui_impl_opengl3.h"

#include "ImGuiEntityOutliner.h"
#include "ImGuiEntityInspector.h"
#include "ImGuiAssetBrowser.h"

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

    // Apply O3DE dark theme colors
    // O3DE accent green: #4a7a4d
    ImVec4* colors = ImGui::GetStyle().Colors;
    
    // Background colors
    colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.95f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.95f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    
    // Text colors
    colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    
    // Headers and titles (O3DE accent green)
    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.29f, 0.48f, 0.30f, 1.0f);  // O3DE Green #4a7a4d
    
    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.29f, 0.48f, 0.30f, 1.0f);  // O3DE Green
    
    // Title bar
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    
    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.29f, 0.48f, 0.30f, 1.0f);  // O3DE Green
    
    // Separator and borders
    colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.29f, 0.48f, 0.30f, 0.5f);  // O3DE Green
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.29f, 0.48f, 0.30f, 1.0f);  // O3DE Green
    
    // Selection and highlights
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.29f, 0.48f, 0.30f, 0.5f);  // O3DE Green
    
    // Check mark
    colors[ImGuiCol_CheckMark] = ImVec4(0.29f, 0.48f, 0.30f, 1.0f);  // O3DE Green
    
    // Slider
    colors[ImGuiCol_SliderGrab] = ImVec4(0.29f, 0.48f, 0.30f, 0.8f);  // O3DE Green
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.29f, 0.48f, 0.30f, 1.0f);  // O3DE Green
    
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