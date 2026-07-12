/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WaylandApplication.h"

#include <AzCore/PlatformIncl.h>

namespace AzFramework
{
    WaylandApplication::WaylandApplication()
    {
        LinuxLifecycleEvents::Bus::Handler::BusConnect();
    }

    WaylandApplication::~WaylandApplication()
    {
        LinuxLifecycleEvents::Bus::Handler::BusDisconnect();
    }

    void WaylandApplication::PumpSystemEventLoopOnce()
    {
        // Qt handles Wayland event processing internally via its platform plugin.
        // For standalone (non-Qt) applications, this would use wl_display_dispatch().
    }

    void WaylandApplication::PumpSystemEventLoopUntilEmpty()
    {
        // Qt handles Wayland event processing internally.
    }
}
