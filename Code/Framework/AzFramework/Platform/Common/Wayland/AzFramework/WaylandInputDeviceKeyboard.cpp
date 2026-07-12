/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WaylandInputDeviceKeyboard.h"

namespace AzFramework
{
    WaylandInputDeviceKeyboard::~WaylandInputDeviceKeyboard() = default;

    bool WaylandInputDeviceKeyboard::IsConnected() const
    {
        return true;
    }

    bool WaylandInputDeviceKeyboard::HasTextEntryStarted() const
    {
        return m_hasTextEntryStarted;
    }

    void WaylandInputDeviceKeyboard::TextEntryStart([[maybe_unused]] const InputDeviceKeyboard::VirtualKeyboardOptions& options)
    {
        m_hasTextEntryStarted = true;
    }

    void WaylandInputDeviceKeyboard::TextEntryStop()
    {
        m_hasTextEntryStarted = false;
    }

    void WaylandInputDeviceKeyboard::TickInputDevice()
    {
        // Input is handled by Qt's Wayland platform plugin in editor mode.
        // For standalone mode, this would use libinput or wayland keyboard protocol.
    }

    void WaylandInputDeviceKeyboard::ResetStoredInputStates()
    {
    }
}
