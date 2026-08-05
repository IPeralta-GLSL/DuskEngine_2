/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Editor/Core/QtEditorApplication.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#endif

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
using xcb_connection_t = struct xcb_connection_t;
#endif

namespace Editor
{
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
    class EditorQtApplicationXcb
        : public EditorQtApplication
        , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
    {
        Q_OBJECT
    public:
        EditorQtApplicationXcb(int& argc, char** argv)
            : EditorQtApplication(argc, argv)
        {
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        }

        xcb_connection_t* GetXcbConnectionFromQt();

        void OnStartPlayInEditor() override;
        void OnStopPlayInEditor() override;

        bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;
    };
#endif

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
    class EditorQtApplicationWayland
        : public EditorQtApplication
        , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
    {
        Q_OBJECT
    public:
        EditorQtApplicationWayland(int& argc, char** argv)
            : EditorQtApplication(argc, argv)
        {
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        }

        ~EditorQtApplicationWayland() override
        {
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        }

        void OnStartPlayInEditor() override;
        void OnStopPlayInEditor() override;

        bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;
    };
#endif
} // namespace Editor
