#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntitySearchBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorEntityAPI.h>

namespace ImGuiEntityOutliner
{
    // Initialize and shutdown
    void Initialize();
    void Shutdown();

    // Main render function
    void Render();

    // Selection API
    AZ::EntityId GetSelectedEntityId();
    const AZStd::vector<AZ::EntityId>& GetSelectedEntities();
    void SetSelectedEntity(AZ::EntityId entityId);
    void ClearSelection();
    void SelectEntity(AZ::EntityId entityId);
    void RefreshSelection();

    // Entity operations
    void CreateNewEntity();
    void DeleteSelectedEntities();
    void DuplicateSelectedEntities();

    // Refresh tree when entities change
    void OnEntityCreated(AZ::EntityId entityId);
    void OnEntityDeleted(AZ::EntityId entityId);
    void OnEntityNameChanged(AZ::EntityId entityId, const AZStd::string& name);
    void OnEntitySelectionChanged(const AZStd::vector<AZ::EntityId>& selected, const AZStd::vector<AZ::EntityId>& deselected);
}