#include "EditorDefs.h"
#include "ImGuiEntityInspector.h"

#include <imgui/imgui.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/EBus/EBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityAPIBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>

namespace ImGuiEntityInspector
{
    static bool s_showAttributesWindow = true;
    static AZ::EntityId s_targetEntity;

    // Inline editing state
    static bool s_isEditingName = false;
    static char s_editNameBuffer[256] = "";

    // O3DE accent green color #4a7a4d
    static const ImVec4 O3DE_ACCENT_GREEN = ImVec4(0.29f, 0.48f, 0.30f, 1.0f);
    static const ImVec4 O3DE_TEXT_NORMAL = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    static const ImVec4 O3DE_TEXT_MUTED = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    static const ImVec4 O3DE_HEADER_BG = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    static const ImVec4 O3DE_BTN_RED = ImVec4(0.5f, 0.15f, 0.15f, 1.0f);
    static const ImVec4 O3DE_BTN_RED_HOVER = ImVec4(0.6f, 0.2f, 0.2f, 1.0f);

    void Initialize()
    {
        s_targetEntity = AZ::EntityId();
        s_isEditingName = false;
        memset(s_editNameBuffer, 0, sizeof(s_editNameBuffer));
    }

    void Shutdown()
    {
    }

    void SetTargetEntity(AZ::EntityId entityId)
    {
        s_targetEntity = entityId;
    }

    AZ::EntityId GetTargetEntity()
    {
        return s_targetEntity;
    }

    void RefreshTargetEntity()
    {
        // Sync with current selection in editor
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
            selectedEntities,
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

        if (!selectedEntities.empty())
        {
            s_targetEntity = selectedEntities[0];
        }
        else
        {
            s_targetEntity = AZ::EntityId();
        }
    }

    void SyncWithStage(AZ::EntityId entityId)
    {
        s_targetEntity = entityId;
    }

    // Check if entity is a valid editor entity
    bool IsValidEditorEntity(AZ::EntityId entityId)
    {
        bool isEditorEntity = false;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            isEditorEntity,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::IsEditorEntity,
            entityId);
        return isEditorEntity;
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

    // Get entity from component
    AZ::Entity* GetEntity(AZ::EntityId entityId)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
        return entity;
    }

    // Get transform component data
    bool GetTransformData(AZ::EntityId entityId, AZ::Vector3& outTranslation, AZ::Vector3& outRotation, float& outUniformScale)
    {
        // Get world transform from transform component
        AZ::TransformBus::EventResult(
            outTranslation, entityId,
            &AZ::TransformBus::Events::GetWorldTranslation);

        // Get world rotation as euler angles (radians), then convert to degrees
        AZ::Vector3 rotRadians;
        AZ::TransformBus::EventResult(
            rotRadians, entityId,
            &AZ::TransformBus::Events::GetWorldRotation);
        outRotation = AZ::Vector3(
            AZ::RadToDeg(rotRadians.GetX()),
            AZ::RadToDeg(rotRadians.GetY()),
            AZ::RadToDeg(rotRadians.GetZ()));

        // Get uniform scale
        AZ::TransformBus::EventResult(
            outUniformScale, entityId,
            &AZ::TransformBus::Events::GetWorldUniformScale);

        return true;
    }

    // Set transform component data
    void SetTransformData(AZ::EntityId entityId, const AZ::Vector3& translation, const AZ::Vector3& rotation, float uniformScale)
    {
        AZ::TransformBus::Event(
            entityId,
            &AZ::TransformBus::Events::SetWorldTranslation, translation);

        // Convert degrees to radians for quaternion
        AZ::Vector3 rotRadians(
            AZ::DegToRad(rotation.GetX()),
            AZ::DegToRad(rotation.GetY()),
            AZ::DegToRad(rotation.GetZ()));
        AZ::Quaternion rotationQuat = AZ::Quaternion::CreateFromEulerRadiansXYZ(rotRadians);
        AZ::TransformBus::Event(
            entityId,
            &AZ::TransformBus::Events::SetWorldRotationQuaternion, rotationQuat);
    }

    // Get component display name from type ID using RTTI
    AZStd::string GetComponentName(AZ::Component* component)
    {
        if (!component)
            return "Unknown";

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        if (serializeContext)
        {
            AZ::TypeId typeId = component->RTTI_GetType();
            const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(typeId);
            if (classData && classData->m_editData)
            {
                // Find display name from edit context
                const AZ::Edit::ElementData* elementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                if (elementData)
                {
                    // Use class data name directly from edit context
                    return classData->m_editData->m_name;
                }
                return classData->m_name;
            }
        }

        // Fallback: use type name
        const char* typeName = component->RTTI_GetTypeName();
        if (typeName)
        {
            // Extract simple name from full type name
            AZStd::string name(typeName);
            size_t lastColon = name.rfind("::");
            if (lastColon != AZStd::string::npos)
            {
                name = name.substr(lastColon + 2);
            }
            return name;
        }

        return AZStd::string::format("Component (%s)", component->RTTI_GetType().template ToString<AZStd::string>(false, false).c_str());
    }

    void RenderTransformSection()
    {
        if (!s_targetEntity.IsValid())
            return;

        ImGui::PushStyleColor(ImGuiCol_Header, O3DE_HEADER_BG);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, O3DE_ACCENT_GREEN);

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PopStyleColor(3);

            ImGui::Indent();

            // Get current transform values
            AZ::Vector3 translation(0, 0, 0);
            AZ::Vector3 rotation(0, 0, 0);
            float uniformScale = 1.0f;
            GetTransformData(s_targetEntity, translation, rotation, uniformScale);

            // Position
            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
            ImGui::Text("Position");
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_NORMAL);
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

            float posValues[3] = { translation.GetX(), translation.GetY(), translation.GetZ() };
            if (ImGui::InputFloat3("##Position", posValues, "%.2f"))
            {
                translation.Set(posValues[0], posValues[1], posValues[2]);
                SetTransformData(s_targetEntity, translation, rotation, uniformScale);
            }

            ImGui::PopStyleColor(3);

            // Rotation (degrees)
            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
            ImGui::Text("Rotation (deg)");
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_NORMAL);
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

            float rotValues[3] = { rotation.GetX(), rotation.GetY(), rotation.GetZ() };
            if (ImGui::InputFloat3("##Rotation", rotValues, "%.2f"))
            {
                rotation.Set(rotValues[0], rotValues[1], rotValues[2]);
                SetTransformData(s_targetEntity, translation, rotation, uniformScale);
            }

            ImGui::PopStyleColor(3);

            // Scale (uniform)
            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
            ImGui::Text("Uniform Scale");
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_NORMAL);
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

            if (ImGui::InputFloat("##Scale", &uniformScale, 0.1f, 1.0f, "%.3f"))
            {
                // Scale can't be set back easily in this simplified version
            }

            ImGui::PopStyleColor(3);

            ImGui::Unindent();
        }
        else
        {
            ImGui::PopStyleColor(3);
        }
    }

    void RenderComponentsSection()
    {
        if (!s_targetEntity.IsValid())
            return;

        ImGui::PushStyleColor(ImGuiCol_Header, O3DE_HEADER_BG);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, O3DE_ACCENT_GREEN);

        if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PopStyleColor(3);

            ImGui::Indent();

            // Get entity and its components
            AZ::Entity* entity = GetEntity(s_targetEntity);
            if (entity)
            {
                const AZ::Entity::ComponentArrayType& components = entity->GetComponents();

                if (components.empty())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
                    ImGui::Text("No components");
                    ImGui::PopStyleColor();
                }
                else
                {
                    // For each component, render an expandable section
                    for (int i = 0; i < components.size(); ++i)
                    {
                        AZ::Component* component = components[i];
                        if (!component)
                            continue;

                        AZStd::string compName = GetComponentName(component);

                        // Component header with remove button
                        ImGui::PushID(i);

                        // Remove button
                        ImGui::PushStyleColor(ImGuiCol_Button, O3DE_BTN_RED);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, O3DE_BTN_RED_HOVER);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, O3DE_ACCENT_GREEN);
                        if (ImGui::SmallButton("X"))
                        {
                            AzToolsFramework::EntityCompositionRequestBus::Broadcast(
                                &AzToolsFramework::EntityCompositionRequests::RemoveComponents,
                                AZStd::span<AZ::Component* const>(&component, 1));
                        }
                        ImGui::PopStyleColor(3);
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Remove %s", compName.c_str());
                        }
                        ImGui::SameLine();

                        // Component name in accent color
                        ImGui::PushStyleColor(ImGuiCol_Text, O3DE_ACCENT_GREEN);
                        bool compOpen = ImGui::CollapsingHeader(compName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
                        ImGui::PopStyleColor();

                        if (compOpen)
                        {
                            ImGui::Indent();

                            // Get component info from SerializeContext
                            AZ::SerializeContext* serializeContext = nullptr;
                            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

                            if (serializeContext)
                            {
                                AZ::TypeId typeId = component->RTTI_GetType();
                                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(typeId);

                                if (classData)
                                {
                                    // Show component type ID
                                    ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
                                    ImGui::Text("Type: %s", classData->m_name);
                                    ImGui::PopStyleColor();

                                    // Show version info
                                    ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
                                    ImGui::Text("Version: %u", classData->m_version);
                                    ImGui::PopStyleColor();

                                    // Show serialize fields
                                    bool hasFields = false;
                                    for (const AZ::SerializeContext::ClassElement& element : classData->m_elements)
                                    {
                                        if (!hasFields)
                                        {
                                            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
                                            ImGui::Text("Fields:");
                                            ImGui::PopStyleColor();
                                            hasFields = true;
                                        }

                                        AZStd::string fieldName = element.m_name ? element.m_name : "unnamed";
                                        ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_NORMAL);
                                        ImGui::BulletText("%s", fieldName.c_str());
                                        ImGui::PopStyleColor();
                                    }
                                }
                                else
                                {
                                    ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
                                    ImGui::Text("%s", component->RTTI_GetTypeName());
                                    ImGui::PopStyleColor();
                                }
                            }

                            ImGui::Unindent();
                        }

                        ImGui::PopID();
                    }
                }
            }

            ImGui::Unindent();

            ImGui::Spacing();

            // Add Component button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, O3DE_ACCENT_GREEN);

            if (ImGui::Button("+ Add Component", ImVec2(-1, 0)))
            {
                // TODO: Open component browser dialog
            }

            ImGui::PopStyleColor(3);
        }
        else
        {
            ImGui::PopStyleColor(3);
        }
    }

    void RenderEntityInfoSection()
    {
        if (!s_targetEntity.IsValid())
            return;

        ImGui::PushStyleColor(ImGuiCol_Header, O3DE_HEADER_BG);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, O3DE_ACCENT_GREEN);

        if (ImGui::CollapsingHeader("Entity Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PopStyleColor(3);

            ImGui::Indent();

            AZStd::string entityName = GetEntityName(s_targetEntity);

            // Inline editable name
            if (s_isEditingName)
            {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_NORMAL);

                ImGui::SetKeyboardFocusHere();
                if (ImGui::InputText("##NameEdit", s_editNameBuffer, IM_ARRAYSIZE(s_editNameBuffer),
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                {
                    if (strlen(s_editNameBuffer) > 0)
                    {
                        AzToolsFramework::EditorEntityAPIBus::Event(
                            s_targetEntity,
                            &AzToolsFramework::EditorEntityAPIRequests::SetName,
                            AZStd::string(s_editNameBuffer));
                    }
                    s_isEditingName = false;
                }
                // Click elsewhere to cancel editing
                if (ImGui::IsItemDeactivated())
                {
                    s_isEditingName = false;
                }

                ImGui::PopStyleColor(2);
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
                ImGui::Text("Name:");
                ImGui::PopStyleColor();
                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_NORMAL);
                ImGui::Text("%s", entityName.c_str());
                ImGui::PopStyleColor();

                // Click to start editing
                ImGui::SameLine();
                if (ImGui::SmallButton("Edit"))
                {
                    s_isEditingName = true;
                    azstrncpy(s_editNameBuffer, AZ_ARRAY_SIZE(s_editNameBuffer), entityName.c_str(), entityName.size() + 1);
                }
            }

            // Entity ID display
            AZStd::string entityIdStr = s_targetEntity.ToString();
            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
            ImGui::Text("ID:");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
            ImGui::Text("%s", entityIdStr.c_str());
            ImGui::PopStyleColor();

            ImGui::Unindent();
        }
        else
        {
            ImGui::PopStyleColor(3);
        }
    }

    void Render()
    {
        if (!s_showAttributesWindow)
            return;

        ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Attributes##ImGui", &s_showAttributesWindow))
        {
            ImGui::End();
            return;
        }

        // Sync with current editor selection
        RefreshTargetEntity();

        // Header with O3DE accent
        ImGui::PushStyleColor(ImGuiCol_Text, O3DE_ACCENT_GREEN);
        ImGui::Text("ATTRIBUTES");
        ImGui::PopStyleColor();

        ImGui::Separator();

        // Entity header
        if (s_targetEntity.IsValid() && IsValidEditorEntity(s_targetEntity))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_ACCENT_GREEN);
            AZStd::string entityName = GetEntityName(s_targetEntity);
            ImGui::Text("%s", entityName.c_str());
            ImGui::PopStyleColor();

            ImGui::Separator();

            // Render sections
            RenderEntityInfoSection();
            RenderTransformSection();
            RenderComponentsSection();

            ImGui::Separator();

            // Action buttons
            ImGui::PushStyleColor(ImGuiCol_Button, O3DE_BTN_RED);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, O3DE_BTN_RED_HOVER);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, O3DE_ACCENT_GREEN);

            if (ImGui::Button("Delete Entity", ImVec2(-1, 0)))
            {
                AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequests::Bus::Events::DeleteEntities,
                    AzToolsFramework::EntityIdList{ s_targetEntity });
                s_targetEntity = AZ::EntityId();
            }

            ImGui::PopStyleColor(3);
        }
        else if (s_targetEntity.IsValid())
        {
            // Entity exists but is not an editor entity (e.g., prefab internal)
            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
            ImGui::Text("Selected entity is internal");
            ImGui::Text("(Prefab system entity)");
            ImGui::PopStyleColor();
        }
        else
        {
            // No entity selected
            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
            ImGui::Text("No entity selected");
            ImGui::Text("Select an entity in Stage");
            ImGui::PopStyleColor();

            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
            ImGui::Text("Tip: Use the viewport to select");
            ImGui::Text("entities or click in the Stage");
            ImGui::Text("hierarchy panel.");
            ImGui::PopStyleColor();
        }

        ImGui::End();
    }
} // namespace ImGuiEntityInspector