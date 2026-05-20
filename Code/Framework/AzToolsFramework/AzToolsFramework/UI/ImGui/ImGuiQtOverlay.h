#pragma once

#include <QOpenGLWidget>
#include <QTimer>

struct ImGuiContext;

namespace AzToolsFramework
{

enum class ImGuiOverlayContent
{
    Stage,
    Attributes,
    AssetBrowser
};

class ImGuiQtOverlay : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit ImGuiQtOverlay(ImGuiOverlayContent contentMode, QWidget* parent = nullptr);
    ~ImGuiQtOverlay();

    ImGuiOverlayContent GetContentMode() const { return m_contentMode; }

signals:
    void ImGuiInitialized();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void SetupImGui();
    void RenderImGuiContent();
    void ShutdownImGui();

    ImGuiContext* m_imguiContext = nullptr;
    QTimer* m_renderTimer = nullptr;
    ImGuiOverlayContent m_contentMode;
};

} // namespace AzToolsFramework