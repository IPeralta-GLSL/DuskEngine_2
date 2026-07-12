/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "QtEditorApplication_linux.h"

#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
#include <AzFramework/WaylandSurfaceAccess.h>
#include <qpa/qplatformnativeinterface.h>
#include <QGuiApplication>
#include <QWindow>
#include <QWidget>
#include <QApplication>
#include <QTimer>
#endif

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
#include <AzFramework/XcbEventHandler.h>
#include <AzFramework/XcbConnectionManager.h>
#include <qpa/qplatformnativeinterface.h>
#endif

namespace Editor
{
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
    static struct wl_surface* WaylandSurfaceCallback(struct wl_display* display)
    {
        QPlatformNativeInterface* native = QGuiApplication::platformNativeInterface();
        if (!native || !display) return nullptr;

        // Force all top-level widgets to create their native windows
        for (QWidget* widget : QApplication::topLevelWidgets())
        {
            if (!widget->isVisible()) continue;
            WId id = widget->winId();
            if (!id) continue;
            QWindow* wh = widget->windowHandle();
            if (!wh) continue;
            wh->create();
            auto* surface = static_cast<struct wl_surface*>(
                native->nativeResourceForWindow(QByteArray("surface"), wh));
            if (surface) return surface;
        }

        // Try all widgets, force winId creation
        for (QWidget* widget : QApplication::allWidgets())
        {
            if (!widget->isVisible()) continue;
            WId id = widget->winId();
            if (!id) continue;
            QWindow* wh = widget->windowHandle();
            if (!wh) continue;
            auto* surface = static_cast<struct wl_surface*>(
                native->nativeResourceForWindow(QByteArray("surface"), wh));
            if (surface) return surface;
        }

        return nullptr;
    }

    static void PopulateWaylandHandles()
    {
        QPlatformNativeInterface* native = QGuiApplication::platformNativeInterface();
        if (!native) return;

        auto* display = static_cast<struct wl_display*>(
            native->nativeResourceForIntegration(QByteArray("wl_display")));
        if (display)
        {
            AzFramework::WaylandSurfaceAccess::SetWaylandDisplay(display);
        }

        AzFramework::WaylandSurfaceAccess::SetWaylandHandleCallback(WaylandSurfaceCallback);

        // Try immediate population
        for (QWidget* widget : QApplication::allWidgets())
        {
            QWindow* wh = widget->windowHandle();
            if (!wh) continue;
            auto* surface = static_cast<struct wl_surface*>(
                native->nativeResourceForWindow(QByteArray("surface"), wh));
            if (surface)
            {
                AzFramework::WaylandSurfaceAccess::SetWaylandSurface(surface);
                break;
            }
        }
    }
#endif

    EditorQtApplication* EditorQtApplication::newInstance(int& argc, char** argv)
    {
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        return new EditorQtApplicationXcb(argc, argv);
#elif PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
        auto* app = new EditorQtApplicationWayland(argc, argv);
        QTimer::singleShot(0, app, []() { PopulateWaylandHandles(); });
        return app;
#endif
        return nullptr;
    }

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
    xcb_connection_t* EditorQtApplicationXcb::GetXcbConnectionFromQt()
    {
        QPlatformNativeInterface* native = platformNativeInterface();
        AZ_Warning("EditorQtApplicationXcb", native, "Unable to retrieve the native platform interface");
        if (!native)
        {
            return nullptr;
        }
        return reinterpret_cast<xcb_connection_t*>(native->nativeResourceForIntegration(QByteArray("connection")));
    }

    void EditorQtApplicationXcb::OnStartPlayInEditor()
    {
        auto* interface = AzFramework::XcbConnectionManagerInterface::Get();
        interface->SetEnableXInput(GetXcbConnectionFromQt(), true);
    }

    void EditorQtApplicationXcb::OnStopPlayInEditor()
    {
        auto* interface = AzFramework::XcbConnectionManagerInterface::Get();
        interface->SetEnableXInput(GetXcbConnectionFromQt(), false);
        AzFramework::XcbEventHandlerBus::Broadcast(&AzFramework::XcbEventHandler::ResetStoredInputStates);
    }

    bool EditorQtApplicationXcb::nativeEventFilter(const QByteArray& eventType, void* message, long*)
    {
        if (GetIEditor()->IsInGameMode())
        {
            AzFramework::XcbEventHandlerBus::Broadcast(
                &AzFramework::XcbEventHandler::HandleXcbEvent, static_cast<xcb_generic_event_t*>(message));

            const auto event = static_cast<xcb_generic_event_t*>(message);
            if ((event->response_type & AzFramework::s_XcbResponseTypeMask) == XCB_CLIENT_MESSAGE)
            {
                return false;
            }

            auto systemCursorState = AzFramework::SystemCursorState::Unknown;
            AzFramework::InputSystemCursorRequestBus::EventResult(systemCursorState, AzFramework::InputDeviceMouse::Id, &AzFramework::InputSystemCursorRequestBus::Events::GetSystemCursorState);
            if(systemCursorState == AzFramework::SystemCursorState::UnconstrainedAndVisible)
            {
                return false;
            }
            return true;
        }
        return false;
    }
#endif

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
    void EditorQtApplicationWayland::OnStartPlayInEditor()
    {
    }

    void EditorQtApplicationWayland::OnStopPlayInEditor()
    {
    }

    bool EditorQtApplicationWayland::nativeEventFilter(
        [[maybe_unused]] const QByteArray& eventType, [[maybe_unused]] void* message, [[maybe_unused]] long* result)
    {
        if (GetIEditor()->IsInGameMode())
        {
            auto systemCursorState = AzFramework::SystemCursorState::Unknown;
            AzFramework::InputSystemCursorRequestBus::EventResult(
                systemCursorState, AzFramework::InputDeviceMouse::Id,
                &AzFramework::InputSystemCursorRequestBus::Events::GetSystemCursorState);
            if (systemCursorState == AzFramework::SystemCursorState::UnconstrainedAndVisible)
            {
                return false;
            }
            return true;
        }
        return false;
    }
#endif
} // namespace Editor
