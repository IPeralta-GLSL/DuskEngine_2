/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace LmbrCentral
{
    class EditorFolderComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:

        AZ_COMPONENT(EditorFolderComponent, "{A7B7B7A0-3F2C-4E2A-8D9F-4C6B5A3E1D0F}",
                     AzToolsFramework::Components::EditorComponentBase);

        void Activate() override {};
        void Deactivate() override {};

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("EditorFolderService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("EditorFolderService"));
        }

        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace LmbrCentral
