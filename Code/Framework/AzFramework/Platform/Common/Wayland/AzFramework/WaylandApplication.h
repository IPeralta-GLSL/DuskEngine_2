/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <AzFramework/Application/Application.h>

namespace AzFramework
{
    class WaylandApplication
        : public Application::Implementation
        , public LinuxLifecycleEvents::Bus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(WaylandApplication, AZ::SystemAllocator);
        WaylandApplication();
        ~WaylandApplication() override;

        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;
    };
}
