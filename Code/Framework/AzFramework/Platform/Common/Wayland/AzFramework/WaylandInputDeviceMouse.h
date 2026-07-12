/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

namespace AzFramework
{
    class WaylandInputDeviceMouse
        : public InputDeviceMouse::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(WaylandInputDeviceMouse, AZ::SystemAllocator);

        WaylandInputDeviceMouse(InputDeviceMouse& inputDevice);
        ~WaylandInputDeviceMouse() override;

        static WaylandInputDeviceMouse::Implementation* Create(InputDeviceMouse& inputDevice);

    protected:
        bool IsConnected() const override;
        void SetSystemCursorState(SystemCursorState systemCursorState) override;
        SystemCursorState GetSystemCursorState() const override;
        void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) override;
        void TickInputDevice() override;
        void ResetStoredInputStates();
        AZ::Vector2 GetSystemCursorPositionNormalized() const override;

    private:
        SystemCursorState m_systemCursorState = SystemCursorState::UnconstrainedAndVisible;
    };
}
