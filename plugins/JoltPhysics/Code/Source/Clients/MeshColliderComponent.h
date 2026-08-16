#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <Atom/RPI.Reflect/Model/ModelAsset.h>

#include <Clients/BaseColliderComponent.h>

namespace JoltPhysics
{
    class MeshColliderComponent
        : public BaseColliderComponent
        , private AZ::EntityBus::Handler
    {
    public:
        AZ_COMPONENT(MeshColliderComponent, "{9A5C2E41-7B3D-4E18-9C21-6F4D3A8B2E17}", BaseColliderComponent);

        enum class MeshShapeType : AZ::u8
        {
            ConvexHull,
            TriangleMesh,
        };

        static void Reflect(AZ::ReflectContext* context);

        void SetMeshAsset(const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset);
        void SetMeshShapeType(MeshShapeType shapeType);
        void SetColliderConfiguration(const Physics::ColliderConfiguration& colliderConfiguration);
        void SetUseMeshComponent(bool useMeshComponent);

    protected:
        void Activate() override;
        void Deactivate() override;
        void UpdateScaleForShapeConfigs() override;

        void OnEntityActivated(const AZ::EntityId& entityId) override;

    private:
        bool BuildShapeConfiguration(const AZ::RPI::ModelAsset& modelAsset);

        AZ::Data::Asset<AZ::RPI::ModelAsset> m_modelAsset;
        MeshShapeType m_meshShapeType = MeshShapeType::ConvexHull;
        Physics::ColliderConfiguration m_colliderConfiguration;
        bool m_useMeshComponent = true;
    };
}
