#pragma once

#include <QOpenGLWidget>
#include <QTimer>

struct ImGuiContext;

namespace AzToolsFramework
{

class ImGuiQtOverlay : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit ImGuiQtOverlay(QWidget* parent = nullptr);
    ~ImGuiQtOverlay();

    void SetStageVisible(bool visible);
    void SetAttributesVisible(bool visible);
    bool IsStageVisible() const;
    bool IsAttributesVisible() const;

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
    bool m_stageVisible = true;
    bool m_attributesVisible = true;
};

} // namespace AzToolsFramework