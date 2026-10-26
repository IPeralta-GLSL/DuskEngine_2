/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Script/ScriptTimePoint.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/Feature/ImGui/SystemBus.h>
#include <AtomLyIntegration/AtomViewportDisplayInfo/AtomViewportInfoDisplayBus.h>

struct ImDrawList;
struct ImFont;

namespace AZ
{
    namespace RPI
    {
        class RenderPipeline;
    } // namespace RPI

    namespace Render
    {
        //! Draws the information overlay on top of the viewports (frame rate, memory usage, pass and
        //! renderer statistics and camera state).
        //! The overlay is drawn with ImGui inside the ImGui pass of each viewport, so it keeps the
        //! same size on screen whatever the internal render resolution of the viewport is, and the
        //! text is rasterized at the resolution it is rendered at. It is drawn without any window
        //! decoration, so it looks like a plain HUD on top of the image.
        //! Which sections are shown is controlled by the Viewport Debug Information actions of the
        //! viewport (r_displayInfo).
        class AtomViewportDisplayInfoSystemComponent
            : public AZ::Component
            , public AZ::TickBus::Handler
            , public AZ::Render::ImGuiSystemNotificationBus::Handler
            , public AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Handler
        {
        public:
            AZ_COMPONENT(AtomViewportDisplayInfoSystemComponent, "{AC32F173-E7E2-4943-8E6C-7C3091978221}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // AZ::Component overrides...
            void Activate() override;
            void Deactivate() override;

            // AZ::TickBus::Handler overrides...
            void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;

            // AZ::Render::ImGuiSystemNotificationBus::Handler overrides...
            void ActiveImGuiContextChanged(ImGuiContext* context) override;
            void OnImGuiPassFrame(ImGuiPass* imguiPass) override;

            // AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Handler overrides...
            AtomBridge::ViewportInfoDisplayState GetDisplayState() const override;
            void SetDisplayState(AtomBridge::ViewportInfoDisplayState state) override;

        private:
            //! A single line of the overlay.
            struct Line
            {
                AZStd::string m_text;
                AZ::Color m_color;
            };

            void UpdateFramerate();

            void DrawFramerate(AZStd::vector<Line>& lines) const;
            void DrawMemoryInfo(AZStd::vector<Line>& lines) const;
            void DrawPassInfo(AZStd::vector<Line>& lines) const;
            void DrawRendererInfo(AZStd::vector<Line>& lines, const AzFramework::WindowSize& renderSize, RPI::RenderPipeline* pipeline) const;
            void DrawCameraInfo(AZStd::vector<Line>& lines, RPI::RenderPipeline* pipeline) const;

            //! Draws the given lines with the given background color behind them.
            void DrawSection(const AZStd::vector<Line>& lines, const AZ::Color& backgroundColor);

            //! Pushes a line to the current section.
            static void AddLine(AZStd::vector<Line>& lines, AZStd::string text, const AZ::Color& color);

            AZStd::string m_rendererDescription;
            AZStd::chrono::duration<double> m_fpsInterval = AZStd::chrono::seconds(1);
            AZStd::deque<AZStd::chrono::steady_clock::time_point> m_fpsHistory;
            bool m_updateRootPassQuery = true;

            // Drawing state of the frame currently being built.
            ImDrawList* m_drawList = nullptr;
            ImFont* m_font = nullptr;
            float m_fontSize = 0.0f;
            float m_lineAdvance = 0.0f;
            float m_lineSpacing = 0.0f;
            float m_bgX = 0.0f;
            float m_bgW = 0.0f;
            float m_bgPad = 0.0f;
            float m_positionY = 0.0f;
        };
    } // namespace Render
} // namespace AZ
