#include <Clients/EditorMeshColliderComponent.h>
#include <Clients/MeshColliderComponent.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>

#include <Atom/RPI.Reflect/Model/ModelAsset.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>

namespace JoltPhysics
{
    void EditorMeshColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorMeshColliderComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("MeshAsset", &EditorMeshColliderComponent::m_modelAsset)
                ->Field("MeshShapeType", &EditorMeshColliderComponent::m_meshShapeType)
                ->Field("ColliderConfiguration", &EditorMeshColliderComponent::m_configuration)
                ->Field("UseMeshComponent", &EditorMeshColliderComponent::m_useMeshComponent)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorMeshColliderComponent>(
                    "Mesh Collider", "Creates a collision shape from the model mesh geometry.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Jolt")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &EditorMeshColliderComponent::m_useMeshComponent,
                        "Use Mesh Component", "Automatically use the mesh asset from the Mesh Component on this entity.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMeshColliderComponent::m_modelAsset,
                        "Mesh Asset", "Model asset to generate the collision shape from. Only used when Use Mesh Component is off.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorMeshColliderComponent::GetMeshAssetVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorMeshColliderComponent::m_meshShapeType,
                        "Shape Type", "Simple uses a convex hull of the mesh, Complex uses the exact triangle mesh.")
                        ->EnumAttribute(MeshColliderComponent::MeshShapeType::ConvexHull, "Simple (Convex Hull)")
                        ->EnumAttribute(MeshColliderComponent::MeshShapeType::TriangleMesh, "Complex (Triangle Mesh)")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMeshColliderComponent::m_configuration,
                        "Collider Configuration", "Collider configuration.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void EditorMeshColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsColliderService"));
        provided.push_back(AZ_CRC_CE("PhysicsTriggerService"));
    }

    void EditorMeshColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
        required.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void EditorMeshColliderComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
    }

    void EditorMeshColliderComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorMeshColliderComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    void EditorMeshColliderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto* colliderComponent = gameEntity->CreateComponent<MeshColliderComponent>();
        colliderComponent->SetMeshAsset(m_modelAsset);
        colliderComponent->SetMeshShapeType(m_meshShapeType);
        colliderComponent->SetColliderConfiguration(m_configuration);
        colliderComponent->SetUseMeshComponent(m_useMeshComponent);
    }

    void EditorMeshColliderComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset = m_modelAsset;
        if (m_useMeshComponent)
        {
            AZ::Data::AssetId assetId;
            AZ::Render::MeshComponentRequestBus::EventResult(assetId, GetEntityId(), &AZ::Render::MeshComponentRequestBus::Events::GetModelAssetId);
            if (assetId.IsValid())
            {
                modelAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::ModelAsset>(
                    assetId, AZ::Data::AssetLoadBehavior::PreLoad);
            }
        }

        if (modelAsset && modelAsset.IsReady())
        {
            const AZ::Aabb aabb = modelAsset->GetAabb();
            if (aabb.IsValid())
            {
                debugDisplay.SetColor(AZ::Color(0.0f, 1.0f, 0.0f, 0.6f));
                debugDisplay.DrawWireBox(aabb.GetMin(), aabb.GetMax());
            }
        }
    }
    AZ::Crc32 EditorMeshColliderComponent::GetMeshAssetVisibility() const
    {
        return m_useMeshComponent ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
    }
}
