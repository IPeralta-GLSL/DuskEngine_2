/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ViewportInfoOverlay.h"

#include <AzCore/Console/IConsole.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <Atom/Feature/ImGui/ImGuiPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <imgui/imgui.h>

AZ_CVAR(bool, ed_viewportInfoOverlay, true, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Draws an overlay on top of the editor viewport with the frame rate and the viewport resolutions");

namespace SandboxEditor
{
    void ViewportInfoOverlay::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ViewportInfoOverlay, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void ViewportInfoOverlay::GetProvidedServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
    }

    void ViewportInfoOverlay::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
    }

    void ViewportInfoOverlay::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ViewportInfoOverlay::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void ViewportInfoOverlay::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
        AZ::Render::ImGuiSystemNotificationBus::Handler::BusConnect();
    }

    void ViewportInfoOverlay::Deactivate()
    {
        AZ::Render::ImGuiSystemNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void ViewportInfoOverlay::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        const float frameTimeMs = deltaTime * 1000.0f;
        m_frameTimeMs = m_frameTimeMs > 0.0f ? (m_frameTimeMs * 0.9f + frameTimeMs * 0.1f) : frameTimeMs;
    }

    void ViewportInfoOverlay::ActiveImGuiContextChanged([[maybe_unused]] ImGuiContext* context)
    {
    }

    void ViewportInfoOverlay::OnImGuiPassFrame(AZ::Render::ImGuiPass* imguiPass)
    {
        if (!ed_viewportInfoOverlay || imguiPass == nullptr)
        {
            return;
        }

        DrawOverlay(imguiPass);
    }

    void ViewportInfoOverlay::DrawOverlay(AZ::Render::ImGuiPass* imguiPass)
    {
        AzFramework::WindowSize clientSize;
        AzFramework::WindowSize renderSize;

        if (const auto* pipeline = imguiPass->GetRenderPipeline())
        {
            if (const auto windowHandle = pipeline->GetWindowHandle())
            {
                AzFramework::WindowRequestBus::EventResult(
                    clientSize, windowHandle, &AzFramework::WindowRequestBus::Events::GetClientAreaSize);
                AzFramework::WindowRequestBus::EventResult(
                    renderSize, windowHandle, &AzFramework::WindowRequestBus::Events::GetRenderResolution);
            }
        }

        // The UI of the overlay is laid out in the on-screen space of the viewport, so it keeps the
        // same size on screen whatever the internal render resolution is.
        const float scale = (clientSize.m_height > 0)
            ? static_cast<float>(renderSize.m_height) / static_cast<float>(clientSize.m_height)
            : 1.0f;

        ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.65f);
        if (ImGui::Begin("Viewport Info", nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::Text("Frame  %.2f ms (%.1f fps)", m_frameTimeMs, m_frameTimeMs > 0.0f ? 1000.0f / m_frameTimeMs : 0.0f);
            ImGui::Separator();
            ImGui::Text("Render %ux%u", renderSize.m_width, renderSize.m_height);
            ImGui::Text("Screen %ux%u", clientSize.m_width, clientSize.m_height);
            ImGui::Text("Scale  %.2fx", scale);
        }
        ImGui::End();
    }
} // namespace SandboxEditor
