/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WaylandInputDeviceMouse.h"

namespace AzFramework
{
    WaylandInputDeviceMouse::~WaylandInputDeviceMouse() = default;

    WaylandInputDeviceMouse::WaylandInputDeviceMouse(InputDeviceMouse& inputDevice)
        : InputDeviceMouse::Implementation(inputDevice)
    {
    }

    WaylandInputDeviceMouse::Implementation* WaylandInputDeviceMouse::Create(InputDeviceMouse& inputDevice)
    {
        return aznew WaylandInputDeviceMouse(inputDevice);
    }

    bool WaylandInputDeviceMouse::IsConnected() const
    {
        return true;
    }

    void WaylandInputDeviceMouse::SetSystemCursorState(SystemCursorState systemCursorState)
    {
        m_systemCursorState = systemCursorState;
    }

    SystemCursorState WaylandInputDeviceMouse::GetSystemCursorState() const
    {
        return m_systemCursorState;
    }

    void WaylandInputDeviceMouse::SetSystemCursorPositionNormalized([[maybe_unused]] AZ::Vector2 positionNormalized)
    {
    }

    void WaylandInputDeviceMouse::TickInputDevice()
    {
        // Input is handled by Qt's Wayland platform plugin in editor mode.
    }

    AZ::Vector2 WaylandInputDeviceMouse::GetSystemCursorPositionNormalized() const
    {
        return AZ::Vector2::CreateZero();
    }

    void WaylandInputDeviceMouse::ResetStoredInputStates()
    {
    }
}
