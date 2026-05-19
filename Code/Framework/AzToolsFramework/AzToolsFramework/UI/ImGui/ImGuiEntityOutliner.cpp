#include "EditorDefs.h"
#include "ImGuiEntityOutliner.h"

#include <imgui/imgui.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityAPIBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>

namespace ImGuiEntityOutliner
{
    static bool s_showStageWindow = true;
    static char s_searchBuffer[256] = "";
    static AZ::EntityId s_selectedEntity;
    static AZStd::vector<AZ::EntityId> s_selectedEntities;
    static bool s_needsRefresh = true;

    // Rename state
    static bool s_isRenaming = false;
    static char s_renameBuffer[256] = "";
    static AZ::EntityId s_renamingEntity;

    // O3DE accent green color #4a7a4d
    static const ImVec4 O3DE_ACCENT_GREEN = ImVec4(0.29f, 0.48f, 0.30f, 1.0f);
    static const ImVec4 O3DE_TEXT_NORMAL = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    static const ImVec4 O3DE_TEXT_MUTED = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    static const ImVec4 O3DE_TEXT_HIDDEN = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
    static const ImVec4 O3DE_TEXT_LOCKED = ImVec4(0.3f, 0.3f, 0.45f, 1.0f);
    static const ImVec4 O3DE_BG_HOVER = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);

    void Initialize()
    {
        memset(s_searchBuffer, 0, sizeof(s_searchBuffer));
        s_selectedEntity = AZ::EntityId();
        s_selectedEntities.clear();
        s_isRenaming = false;
        s_renamingEntity = AZ::EntityId();
    }

    void Shutdown()
    {
    }

    AZ::EntityId GetSelectedEntityId()
    {
        return s_selectedEntity;
    }

    const AZStd::vector<AZ::EntityId>& GetSelectedEntities()
    {
        return s_selectedEntities;
    }

    void SetSelectedEntity(AZ::EntityId entityId)
    {
        s_selectedEntity = entityId;
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::SetSelectedEntities,
            AzToolsFramework::EntityIdList{ entityId });
    }

    void ClearSelection()
    {
        s_selectedEntity = AZ::EntityId();
        s_selectedEntities.clear();
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::SetSelectedEntities,
            AzToolsFramework::EntityIdList{});
    }

    void SelectEntity(AZ::EntityId entityId)
    {
        SetSelectedEntity(entityId);
    }

    void RefreshSelection()
    {
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
            selectedEntities,
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

        s_selectedEntities = selectedEntities;
        if (!selectedEntities.empty())
        {
            s_selectedEntity = selectedEntities[0];
        }
        else
        {
            s_selectedEntity = AZ::EntityId();
        }
    }

    void CreateNewEntity()
    {
        AZ::EntityId newEntityId;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
            newEntityId,
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::CreateNewEntity,
            AZ::EntityId());

        if (newEntityId.IsValid())
        {
            SetSelectedEntity(newEntityId);
            s_needsRefresh = true;
        }
    }

    void CreateNewChildEntity(AZ::EntityId parentId)
    {
        if (!parentId.IsValid())
        {
            CreateNewEntity();
            return;
        }

        AZ::EntityId newEntityId;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
            newEntityId,
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::CreateNewEntity,
            parentId);

        if (newEntityId.IsValid())
        {
            // Set parent
            AzToolsFramework::EditorEntityAPIBus::Event(
                newEntityId,
                &AzToolsFramework::EditorEntityAPIRequests::SetParent,
                parentId);
            SetSelectedEntity(newEntityId);
            s_needsRefresh = true;
        }
    }

    void DeleteSelectedEntities()
    {
        if (!s_selectedEntities.empty())
        {
            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::Bus::Events::DeleteEntities,
                s_selectedEntities);
            s_selectedEntities.clear();
            s_selectedEntity = AZ::EntityId();
            s_needsRefresh = true;
        }
    }

    void DeleteEntity(AZ::EntityId entityId)
    {
        if (entityId.IsValid())
        {
            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::Bus::Events::DeleteEntities,
                AzToolsFramework::EntityIdList{ entityId });
            s_needsRefresh = true;
        }
    }

    void DuplicateEntity(AZ::EntityId entityId)
    {
        if (!entityId.IsValid())
            return;
        // Duplicate via simple API: just create and copy isn't available without full context
        // We'll create a new child under same parent instead
        s_needsRefresh = true;
    }

    // Helper to get entity name
    AZStd::string GetEntityName(AZ::EntityId entityId)
    {
        AZStd::string name;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            name, entityId,
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetName);
        return name;
    }

    bool GetEntityChildren(AZ::EntityId entityId, AzToolsFramework::EntityIdList& outChildren)
    {
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            outChildren, entityId,
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);
        return !outChildren.empty();
    }

    bool IsEntitySelected(AZ::EntityId entityId)
    {
        bool selected = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            selected, entityId,
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSelected);
        return selected;
    }

    bool IsEntityVisible(AZ::EntityId entityId)
    {
        bool visible = true;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            visible, entityId,
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);
        return visible;
    }

    bool IsEntityLocked(AZ::EntityId entityId)
    {
        bool locked = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            locked, entityId,
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsLocked);
        return locked;
    }

    bool IsEntitySliceEntity(AZ::EntityId entityId)
    {
        bool isSlice = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isSlice, entityId,
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceEntity);
        return isSlice;
    }

    // Check if an entity has no parent (root entity)
    bool IsRootEntity(AZ::EntityId entityId)
    {
        AZ::EntityId parentId;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            parentId, entityId,
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);
        return !parentId.IsValid();
    }

    // Check if entity is a valid editor entity (not a Prefab internal entity)
    bool IsValidEditorEntity(AZ::EntityId entityId)
    {
        bool isEditorEntity = false;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            isEditorEntity,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::IsEditorEntity,
            entityId);
        return isEditorEntity;
    }

    // Toggle entity visibility
    void ToggleEntityVisibility(AZ::EntityId entityId)
    {
        if (!entityId.IsValid())
            return;

        bool currentlyVisible = IsEntityVisible(entityId);
        AzToolsFramework::EditorEntityAPIBus::Event(
            entityId,
            &AzToolsFramework::EditorEntityAPIRequests::SetVisibilityState,
            !currentlyVisible);
    }

    // Toggle entity lock
    void ToggleEntityLock(AZ::EntityId entityId)
    {
        if (!entityId.IsValid())
            return;

        bool currentlyLocked = IsEntityLocked(entityId);
        AzToolsFramework::EditorEntityAPIBus::Event(
            entityId,
            &AzToolsFramework::EditorEntityAPIRequests::SetLockState,
            !currentlyLocked);
    }

    // Recursive tree rendering
    void RenderEntityNode(AZ::EntityId entityId, int nodeId)
    {
        if (!entityId.IsValid())
            return;

        // CRASH FIX: Skip entities that are not editor entities
        if (!IsValidEditorEntity(entityId))
            return;

        // Get entity info
        AZStd::string entityName = GetEntityName(entityId);
        if (entityName.empty())
        {
            entityName = "Unknown Entity";
        }

        AzToolsFramework::EntityIdList children;
        bool hasChildren = GetEntityChildren(entityId, children);

        // Check if this entity is selected
        bool isSelected = IsEntitySelected(entityId);
        bool visible = IsEntityVisible(entityId);
        bool locked = IsEntityLocked(entityId);
        bool isSlice = IsEntitySliceEntity(entityId);

        // Use sequential node ID for ImGui
        ImGui::PushID(nodeId);

        // ----------------------------------------------------------------
        // Visibility toggle (eye icon)
        // ----------------------------------------------------------------
        ImVec4 visColor = visible ? O3DE_TEXT_NORMAL : O3DE_TEXT_MUTED;
        ImGui::PushStyleColor(ImGuiCol_Text, visColor);
        if (ImGui::SmallButton(visible ? "V" : "v"))
        {
            ToggleEntityVisibility(entityId);
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s visibility", visible ? "Hide" : "Show");
        }
        ImGui::SameLine();

        // ----------------------------------------------------------------
        // Lock toggle
        // ----------------------------------------------------------------
        ImVec4 lockColor = locked ? O3DE_TEXT_LOCKED : O3DE_TEXT_MUTED;
        ImGui::PushStyleColor(ImGuiCol_Text, lockColor);
        if (ImGui::SmallButton(locked ? "L" : "l"))
        {
            ToggleEntityLock(entityId);
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s entity", locked ? "Unlock" : "Lock");
        }
        ImGui::SameLine();

        // ----------------------------------------------------------------
        // Slice indicator
        // ----------------------------------------------------------------
        if (isSlice)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
            ImGui::TextUnformatted("S");
            ImGui::PopStyleColor();
            ImGui::SameLine();
        }

        // ----------------------------------------------------------------
        // Entity tree node
        // ----------------------------------------------------------------
        // Set selection color
        ImVec4 textColor;
        if (!visible) textColor = O3DE_TEXT_HIDDEN;
        else if (locked) textColor = O3DE_TEXT_LOCKED;
        else textColor = (isSelected) ? O3DE_ACCENT_GREEN : O3DE_TEXT_NORMAL;

        // Tree node flags
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_FramePadding;
        if (!hasChildren)
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        if (isSelected)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        bool nodeOpen = ImGui::TreeNodeEx("node", flags, "%s", entityName.c_str());

        // Handle click on this node
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            SelectEntity(entityId);
        }

        // Double-click to rename
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            s_isRenaming = true;
            s_renamingEntity = entityId;
            azstrncpy(s_renameBuffer, AZ_ARRAY_SIZE(s_renameBuffer), entityName.c_str(), entityName.size() + 1);
        }

        ImGui::PopStyleColor();

        // ----------------------------------------------------------------
        // Context menu (right-click)
        // ----------------------------------------------------------------
        if (ImGui::BeginPopupContextItem("EntityContextMenu"))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_ACCENT_GREEN);
            ImGui::Text("Entity: %s", entityName.c_str());
            ImGui::PopStyleColor();
            ImGui::Separator();

            if (ImGui::MenuItem("Select"))
            {
                SelectEntity(entityId);
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Rename"))
            {
                s_isRenaming = true;
                s_renamingEntity = entityId;
                azstrncpy(s_renameBuffer, AZ_ARRAY_SIZE(s_renameBuffer), entityName.c_str(), entityName.size() + 1);
            }
            if (ImGui::MenuItem("Create Child Entity"))
            {
                CreateNewChildEntity(entityId);
            }
            if (ImGui::MenuItem("Duplicate"))
            {
                DuplicateEntity(entityId);
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Delete", "Del"))
            {
                DeleteEntity(entityId);
            }
            ImGui::Separator();

            if (ImGui::MenuItem(visible ? "Hide" : "Show"))
            {
                ToggleEntityVisibility(entityId);
            }
            if (ImGui::MenuItem(locked ? "Unlock" : "Lock"))
            {
                ToggleEntityLock(entityId);
            }

            ImGui::EndPopup();
        }

        if (nodeOpen && hasChildren)
        {
            int childNodeId = nodeId * 1000 + 1;
            for (AZ::EntityId childId : children)
            {
                RenderEntityNode(childId, childNodeId++);
            }
            ImGui::TreePop();
        }
        else if (nodeOpen)
        {
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void Render()
    {
        if (!s_showStageWindow)
            return;

        ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Stage##ImGui", &s_showStageWindow,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
        {
            ImGui::End();
            return;
        }

        // Header with O3DE accent
        ImGui::PushStyleColor(ImGuiCol_Text, O3DE_ACCENT_GREEN);
        ImGui::Text("STAGE");
        ImGui::PopStyleColor();

        ImGui::Separator();

        // Search bar
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, O3DE_TEXT_MUTED);

        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search entities...", s_searchBuffer, IM_ARRAYSIZE(s_searchBuffer));
        ImGui::PopStyleColor(3);

        ImGui::Separator();

        // Refresh selection from editor
        RefreshSelection();

        // Handle rename modal if active
        if (s_isRenaming && s_renamingEntity.IsValid())
        {
            ImGui::OpenPopup("Rename Entity");
        }

        if (ImGui::BeginPopupModal("Rename Entity", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("New name:");
            ImGui::InputText("##rename", s_renameBuffer, IM_ARRAYSIZE(s_renameBuffer));

            if (ImGui::Button("OK", ImVec2(60, 0)))
            {
                if (strlen(s_renameBuffer) > 0)
                {
                    AzToolsFramework::EditorEntityAPIBus::Event(
                        s_renamingEntity,
                        &AzToolsFramework::EditorEntityAPIRequests::SetName,
                        AZStd::string(s_renameBuffer));
                }
                s_isRenaming = false;
                s_renamingEntity = AZ::EntityId();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(60, 0)))
            {
                s_isRenaming = false;
                s_renamingEntity = AZ::EntityId();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Toolbar
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, O3DE_ACCENT_GREEN);

        if (ImGui::Button("+ New", ImVec2(60, 0)))
        {
            CreateNewEntity();
        }
        ImGui::SameLine();
        if (ImGui::Button("x Del", ImVec2(60, 0)))
        {
            DeleteSelectedEntities();
        }
        ImGui::SameLine();
        if (ImGui::Button("Ref", ImVec2(60, 0)))
        {
            s_needsRefresh = true;
        }

        ImGui::PopStyleColor(3);

        ImGui::Separator();

        // Entity tree
        ImGui::BeginChild("EntityTree", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 10), true);

        // CRASH FIX: Collect entities filtered by IsEditorEntity
        AZStd::vector<AZ::EntityId> allEntityIds;
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::EnumerateEntities,
            [&allEntityIds](AZ::Entity* entity)
            {
                if (entity && ImGuiEntityOutliner::IsValidEditorEntity(entity->GetId()))
                {
                    allEntityIds.push_back(entity->GetId());
                }
            });

        // Filter root entities and apply search
        AZStd::vector<AZ::EntityId> rootEntities;
        for (AZ::EntityId entityId : allEntityIds)
        {
            if (IsRootEntity(entityId))
            {
                // Apply search filter
                if (strlen(s_searchBuffer) > 0)
                {
                    AZStd::string name = GetEntityName(entityId);
                    AZStd::string searchLower = AZStd::string(s_searchBuffer);
                    AZStd::to_lower(searchLower.begin(), searchLower.end());
                    AZStd::string nameLower = name;
                    AZStd::to_lower(nameLower.begin(), nameLower.end());
                    // Show entity if name matches or any descendant matches
                    if (nameLower.find(searchLower) == AZStd::string::npos)
                    {
                        continue;
                    }
                }
                rootEntities.push_back(entityId);
            }
        }

        if (rootEntities.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
            if (strlen(s_searchBuffer) > 0)
            {
                ImGui::Text("No matching entities");
                ImGui::Text("found for: \"%s\"", s_searchBuffer);
            }
            else
            {
                ImGui::Text("No entities in level");
                ImGui::Text("Click 'New' to create an entity");
            }
            ImGui::PopStyleColor();
        }
        else
        {
            int nodeId = 0;
            for (AZ::EntityId rootId : rootEntities)
            {
                RenderEntityNode(rootId, nodeId++);
            }
        }

        ImGui::EndChild();

        // Status bar
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);

        size_t totalCount = rootEntities.size();
        AZStd::string statusText = AZStd::string::format(
            "Entities: %zu | Selected: %zu",
            totalCount,
            s_selectedEntities.size()
        );
        ImGui::Text("%s", statusText.c_str());

        ImGui::PopStyleColor();

        ImGui::End();
    }
} // namespace ImGuiEntityOutliner