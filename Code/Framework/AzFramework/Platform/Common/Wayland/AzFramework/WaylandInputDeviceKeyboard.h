/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>

namespace AzFramework
{
    class WaylandInputDeviceKeyboard
        : public InputDeviceKeyboard::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(WaylandInputDeviceKeyboard, AZ::SystemAllocator);

        using InputDeviceKeyboard::Implementation::Implementation;
        ~WaylandInputDeviceKeyboard() override;

        bool IsConnected() const override;
        bool HasTextEntryStarted() const override;
        void TextEntryStart(const InputDeviceKeyboard::VirtualKeyboardOptions& options) override;
        void TextEntryStop() override;
        void TickInputDevice() override;
        void ResetStoredInputStates();

    private:
        bool m_hasTextEntryStarted = false;
    };
}
