#include <Clients/MeshColliderComponent.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzFramework/Physics/SystemBus.h>

#include <Clients/RigidBody.h>
#include <Clients/StaticRigidBody.h>

#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>

#include <Utils.h>

namespace JoltPhysics
{
    void MeshColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MeshColliderComponent, BaseColliderComponent>()
                ->Version(1)
                ->Field("MeshAsset", &MeshColliderComponent::m_modelAsset)
                ->Field("MeshShapeType", &MeshColliderComponent::m_meshShapeType)
                ->Field("ColliderConfiguration", &MeshColliderComponent::m_colliderConfiguration)
                ->Field("UseMeshComponent", &MeshColliderComponent::m_useMeshComponent)
                ;
        }
    }

    void MeshColliderComponent::SetMeshAsset(const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset)
    {
        m_modelAsset = modelAsset;
    }

    void MeshColliderComponent::SetMeshShapeType(MeshShapeType shapeType)
    {
        m_meshShapeType = shapeType;
    }

    void MeshColliderComponent::SetColliderConfiguration(const Physics::ColliderConfiguration& colliderConfiguration)
    {
        m_colliderConfiguration = colliderConfiguration;
    }

    void MeshColliderComponent::SetUseMeshComponent(bool useMeshComponent)
    {
        m_useMeshComponent = useMeshComponent;
    }

    void MeshColliderComponent::Activate()
    {
        AZ::EntityBus::Handler::BusConnect(GetEntityId());
    }

    void MeshColliderComponent::Deactivate()
    {
        AZ::EntityBus::Handler::BusDisconnect();
        BaseColliderComponent::Deactivate();
    }

    void MeshColliderComponent::OnEntityActivated([[maybe_unused]] const AZ::EntityId& entityId)
    {
        AZ::EntityBus::Handler::BusDisconnect();

        AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset;
        if (m_useMeshComponent)
        {
            AZ::Data::AssetId assetId;
            AZ::Render::MeshComponentRequestBus::EventResult(assetId, GetEntityId(), &AZ::Render::MeshComponentRequestBus::Events::GetModelAssetId);
            if (!assetId.IsValid())
            {
                AZ_Error("Jolt Mesh Collider", false,
                    "No mesh component found on entity \"%s\". Add a Mesh Component or disable Use Mesh Component and assign the mesh asset manually.",
                    GetEntity()->GetName().c_str());
                return;
            }
            modelAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::ModelAsset>(
                assetId, AZ::Data::AssetLoadBehavior::PreLoad);
        }
        else
        {
            modelAsset = m_modelAsset;
            if (modelAsset.GetId().IsValid() && !modelAsset.IsReady())
            {
                modelAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::ModelAsset>(
                    modelAsset.GetId(), AZ::Data::AssetLoadBehavior::PreLoad);
            }
        }

        AZ::RPI::ModelAsset* modelData = modelAsset.Get();
        if (!modelData)
        {
            AZ_Error("Jolt Mesh Collider", false, "Failed to load mesh asset for entity \"%s\".",
                GetEntity()->GetName().c_str());
            return;
        }

        AZ_TracePrintf("JoltMeshCollider", "OnEntityActivated entity=%s lods=%zu", GetEntity()->GetName().c_str(),
            modelData->GetLodCount());

        if (!BuildShapeConfiguration(*modelData))
        {
            AZ_Error("Jolt Mesh Collider", false, "Failed to build shape configuration for entity \"%s\".",
                GetEntity()->GetName().c_str());
            return;
        }

        AZ_TracePrintf("JoltMeshCollider", "Shapes built for entity=%s", GetEntity()->GetName().c_str());

        BaseColliderComponent::Activate();

        AzPhysics::SceneHandle sceneHandle = AzPhysics::InvalidSceneHandle;
        Physics::DefaultWorldBus::BroadcastResult(
            sceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);
        if (sceneHandle == AzPhysics::InvalidSceneHandle)
        {
            return;
        }

        AzPhysics::SimulatedBodyHandle bodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            bodyHandle, GetEntityId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBodyHandle);
        if (bodyHandle == AzPhysics::InvalidSimulatedBodyHandle)
        {
            return;
        }

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            if (auto* body = sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, bodyHandle))
            {
                if (auto* joltRigidBody = azdynamic_cast<JoltPhysics::RigidBody*>(body))
                {
                    for (const auto& shape : GetShapes())
                    {
                        joltRigidBody->AddShape(shape);
                    }
                }
                else if (auto* joltStaticBody = azdynamic_cast<JoltPhysics::StaticRigidBody*>(body))
                {
                    for (const auto& shape : GetShapes())
                    {
                        joltStaticBody->AddShape(shape);
                    }
                }
            }
        }
    }

    void MeshColliderComponent::UpdateScaleForShapeConfigs()
    {
        if (m_shapeConfigList.size() == 1)
        {
            m_shapeConfigList[0].second->m_scale = AZ::Vector3(Utils::GetTransformScale(GetEntityId()));
        }
    }

    bool MeshColliderComponent::BuildShapeConfiguration(const AZ::RPI::ModelAsset& modelAsset)
    {
        if (modelAsset.GetLodCount() == 0)
        {
            AZ_Error("Jolt Mesh Collider", false, "Model asset has no lods.");
            return false;
        }

        const AZ::RPI::ModelLodAsset* lodAsset = modelAsset.GetLodAssets()[0].Get();
        if (!lodAsset)
        {
            AZ_Error("Jolt Mesh Collider", false, "Model asset lod is not loaded.");
            return false;
        }

        AZStd::vector<AZ::Vector3> positions;
        AZStd::vector<AZ::u32> indices;

        static const AZ::Name positionSemantic("POSITION");

        AZ_TracePrintf("JoltMeshCollider", "Lod meshes=%zu", lodAsset->GetMeshes().size());

        for (const AZ::RPI::ModelLodAsset::Mesh& mesh : lodAsset->GetMeshes())
        {
            const AZ::RPI::BufferAssetView* positionView = mesh.GetSemanticBufferAssetView(positionSemantic);
            if (!positionView)
            {
                AZ_Error("Jolt Mesh Collider", false, "Mesh \"%s\" has no position stream.", mesh.GetName().GetStringView().data());
                return false;
            }

            const AZ::RHI::Format positionFormat = positionView->GetBufferViewDescriptor().m_elementFormat;
            AZStd::vector<AZ::Vector3> meshPositions;
            if (positionFormat == AZ::RHI::Format::R32G32B32_FLOAT)
            {
                const AZStd::span<const AZ::Vector3> typedPositions = mesh.GetSemanticBufferTyped<AZ::Vector3>(positionSemantic);
                meshPositions.assign(typedPositions.begin(), typedPositions.end());
            }
            else if (positionFormat == AZ::RHI::Format::R32G32B32A32_FLOAT)
            {
                const AZStd::span<const AZ::Vector4> typedPositions = mesh.GetSemanticBufferTyped<AZ::Vector4>(positionSemantic);
                meshPositions.reserve(typedPositions.size());
                for (const AZ::Vector4& position : typedPositions)
                {
                    meshPositions.emplace_back(position.GetAsVector3());
                }
            }
            else
            {
                AZ_Error("Jolt Mesh Collider", false, "Mesh \"%s\" has unsupported position format.", mesh.GetName().GetStringView().data());
                return false;
            }

            if (meshPositions.empty())
            {
                AZ_Error("Jolt Mesh Collider", false, "Mesh \"%s\" has no position data.", mesh.GetName().GetStringView().data());
                return false;
            }

            const AZ::u32 vertexOffset = static_cast<AZ::u32>(positions.size());
            positions.insert(positions.end(), meshPositions.begin(), meshPositions.end());

            const AZ::RHI::Format indexFormat = mesh.GetIndexBufferAssetView().GetBufferViewDescriptor().m_elementFormat;
            if (indexFormat == AZ::RHI::Format::R32_UINT)
            {
                for (const AZ::u32 index : mesh.GetIndexBufferTyped<AZ::u32>())
                {
                    indices.push_back(vertexOffset + index);
                }
            }
            else if (indexFormat == AZ::RHI::Format::R16_UINT)
            {
                for (const AZ::u16 index : mesh.GetIndexBufferTyped<AZ::u16>())
                {
                    indices.push_back(vertexOffset + index);
                }
            }
            else
            {
                AZ_Error("Jolt Mesh Collider", false, "Mesh \"%s\" has unsupported index format.", mesh.GetName().GetStringView().data());
                return false;
            }
        }

        if (positions.empty() || indices.empty())
        {
            AZ_Error("Jolt Mesh Collider", false, "No geometry found in model asset.");
            return false;
        }

        AZ_TracePrintf("JoltMeshCollider", "Geometry: vertices=%zu indices=%zu", positions.size(), indices.size());

        for (const AZ::Vector3& position : positions)
        {
            if (!position.IsFinite())
            {
                AZ_Error("Jolt Mesh Collider", false, "Mesh contains non-finite vertex data.");
                return false;
            }
        }

        auto colliderConfiguration = AZStd::make_shared<Physics::ColliderConfiguration>(m_colliderConfiguration);
        auto shapeConfiguration = AZStd::make_shared<Physics::CookedMeshShapeConfiguration>();

        bool cooked = false;
        if (m_meshShapeType == MeshShapeType::ConvexHull)
        {
            AZStd::vector<AZ::u8> cookedData;
            Physics::SystemRequestBus::BroadcastResult(cooked,
                &Physics::SystemRequests::CookConvexMeshToMemory,
                positions.data(), static_cast<AZ::u32>(positions.size()), cookedData);
            if (cooked)
            {
                shapeConfiguration->SetCookedMeshData(cookedData.data(), cookedData.size(),
                    Physics::CookedMeshShapeConfiguration::MeshType::Convex);
            }
        }
        else
        {
            AZStd::vector<AZ::u8> cookedData;
            Physics::SystemRequestBus::BroadcastResult(cooked,
                &Physics::SystemRequests::CookTriangleMeshToMemory,
                positions.data(), static_cast<AZ::u32>(positions.size()),
                indices.data(), static_cast<AZ::u32>(indices.size()), cookedData);
            if (cooked)
            {
                shapeConfiguration->SetCookedMeshData(cookedData.data(), cookedData.size(),
                    Physics::CookedMeshShapeConfiguration::MeshType::TriangleMesh);
            }
        }

        if (!cooked)
        {
            AZ_Error("Jolt Mesh Collider", false, "Failed to cook mesh geometry.");
            return false;
        }

        m_shapeConfigList = { AZStd::make_pair(colliderConfiguration, shapeConfiguration) };

        return true;
    }
}
