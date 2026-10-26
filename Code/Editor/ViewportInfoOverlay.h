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
#include <Atom/Feature/ImGui/SystemBus.h>

namespace SandboxEditor
{
    //! Draws an information overlay on top of the editor viewport with ImGui. It shows the frame
    //! rate and the resolutions of the viewport (the internal render resolution, the on-screen size
    //! and the scale between them).
    //! The overlay is drawn inside the ImGui pass of each viewport, so it keeps a constant size on
    //! screen whatever the internal render resolution of the viewport is, and the text is rasterized
    //! at the resolution it is rendered at.
    class ViewportInfoOverlay
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public AZ::Render::ImGuiSystemNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(ViewportInfoOverlay, "{E4EC9CA0-F327-4A66-B26D-87956B8A0B9E}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

    private:
        // AZ::TickBus::Handler overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;

        // AZ::Render::ImGuiSystemNotificationBus::Handler overrides ...
        void ActiveImGuiContextChanged(ImGuiContext* context) override;
        void OnImGuiPassFrame(AZ::Render::ImGuiPass* imguiPass) override;

        void DrawOverlay(AZ::Render::ImGuiPass* imguiPass);

        // Smoothed frame time of the application.
        float m_frameTimeMs = 0.0f;
    };
} // namespace SandboxEditor
