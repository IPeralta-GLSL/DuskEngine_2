/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Windowing/NativeWindow.h>

struct wl_display;
struct wl_surface;

namespace AzFramework
{
    struct WaylandNativeHandles
    {
        wl_display* m_display = nullptr;
        wl_surface* m_surface = nullptr;
    };

    class WaylandNativeWindow final
        : public NativeWindow::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(WaylandNativeWindow, AZ::SystemAllocator);
        WaylandNativeWindow();
        ~WaylandNativeWindow() override;

        void InitWindowInternal(const AZStd::string& title, const WindowGeometry& geometry, const WindowStyleMasks& styleMasks) override;
        void Activate() override;
        void Deactivate() override;
        NativeWindowHandle GetWindowHandle() const override;
        void SetWindowTitle(const AZStd::string& title) override;
        void ResizeClientArea(WindowSize clientAreaSize, const WindowPosOptions& options) override;
        bool SupportsClientAreaResize() const override;
        uint32_t GetDisplayRefreshRate() const override;
        bool GetFullScreenState() const override;
        void SetFullScreenState(bool fullScreenState) override;

        static const WaylandNativeHandles& GetNativeHandles() { return s_handles; }
        static WaylandNativeHandles& MutableNativeHandles() { return s_handles; }

    private:
        void WindowSizeChanged(const uint32_t width, const uint32_t height);
        void QueryNativeHandles();

        bool m_fullscreenState = false;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        static inline WaylandNativeHandles s_handles{};
    };
}
