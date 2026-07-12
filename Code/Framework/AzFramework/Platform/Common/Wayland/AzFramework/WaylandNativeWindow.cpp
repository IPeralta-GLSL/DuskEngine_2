/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WaylandNativeWindow.h"

// Forward declare Qt types to avoid heavy includes
struct wl_display;
struct wl_surface;

namespace AzFramework
{
    WaylandNativeWindow::WaylandNativeWindow() = default;
    WaylandNativeWindow::~WaylandNativeWindow() = default;

    void WaylandNativeWindow::InitWindowInternal(
        [[maybe_unused]] const AZStd::string& title,
        const WindowGeometry& geometry,
        [[maybe_unused]] const WindowStyleMasks& styleMasks)
    {
        m_width = static_cast<uint32_t>(geometry.m_width);
        m_height = static_cast<uint32_t>(geometry.m_height);
        QueryNativeHandles();
    }

    void WaylandNativeWindow::Activate()
    {
        QueryNativeHandles();
        WindowSizeChanged(m_width, m_height);
    }

    void WaylandNativeWindow::QueryNativeHandles()
    {
        if (s_handles.m_display && s_handles.m_surface)
        {
            return;
        }
        // The editor's EditorQtApplicationWayland::nativeEventFilter will
        // populate these handles when Qt windows are ready.
        // This is a fallback for standalone mode.
    }

    void WaylandNativeWindow::Deactivate()
    {
    }

    NativeWindowHandle WaylandNativeWindow::GetWindowHandle() const
    {
        return static_cast<NativeWindowHandle>(s_handles.m_surface);
    }

    void WaylandNativeWindow::SetWindowTitle([[maybe_unused]] const AZStd::string& title)
    {
    }

    void WaylandNativeWindow::ResizeClientArea(WindowSize clientAreaSize, [[maybe_unused]] const WindowPosOptions& options)
    {
        m_width = clientAreaSize.m_width;
        m_height = clientAreaSize.m_height;
        WindowSizeChanged(m_width, m_height);
    }

    bool WaylandNativeWindow::SupportsClientAreaResize() const
    {
        return true;
    }

    uint32_t WaylandNativeWindow::GetDisplayRefreshRate() const
    {
        return 60;
    }

    bool WaylandNativeWindow::GetFullScreenState() const
    {
        return m_fullscreenState;
    }

    void WaylandNativeWindow::SetFullScreenState(bool fullScreenState)
    {
        m_fullscreenState = fullScreenState;
    }

    void WaylandNativeWindow::WindowSizeChanged(const uint32_t width, const uint32_t height)
    {
        m_width = width;
        m_height = height;
        WindowNotificationBus::Event(
            GetWindowHandle(), &WindowNotificationBus::Events::OnWindowResized, width, height);
    }


}
