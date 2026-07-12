/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

struct wl_display;
struct wl_surface;

namespace AzFramework
{
    using WaylandHandleCallback = struct wl_surface*(*)(struct wl_display*);

    namespace WaylandSurfaceAccess
    {
        inline struct wl_display* s_wlDisplay = nullptr;
        inline struct wl_surface* s_wlSurface = nullptr;
        inline WaylandHandleCallback s_callback = nullptr;

        inline void SetWaylandDisplay(struct wl_display* display) { s_wlDisplay = display; }
        inline void SetWaylandSurface(struct wl_surface* surface) { s_wlSurface = surface; }
        inline void SetWaylandHandleCallback(WaylandHandleCallback cb) { s_callback = cb; }

        inline struct wl_display* GetWaylandDisplay() { return s_wlDisplay; }

        inline struct wl_surface* GetWaylandSurface()
        {
            if (s_wlSurface) return s_wlSurface;
            if (s_callback && s_wlDisplay)
            {
                s_wlSurface = s_callback(s_wlDisplay);
            }
            return s_wlSurface;
        }
    }
}
