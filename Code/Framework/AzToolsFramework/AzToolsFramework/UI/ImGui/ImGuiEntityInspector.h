#pragma once

#include <AzCore/Component/EntityId.h>

namespace ImGuiEntityInspector
{
    // Initialize and shutdown
    void Initialize();
    void Shutdown();

    // Main render function
    void Render();

    // Target entity management
    void SetTargetEntity(AZ::EntityId entityId);
    AZ::EntityId GetTargetEntity();
    void RefreshTargetEntity();

    // Sync with Stage selection
    void SyncWithStage(AZ::EntityId entityId);
}