#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Color.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <Atom/RPI.Reflect/Model/ModelAsset.h>

#include <Clients/MeshColliderComponent.h>

namespace JoltPhysics
{
    class EditorMeshColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorMeshColliderComponent, "{B7E9C5A2-8D4F-4A1B-9E63-2C5F6D7A8B91}",
            AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    protected:
        void Activate() override;
        void Deactivate() override;

        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        AZ::Crc32 GetMeshAssetVisibility() const;
        AZ::Data::Asset<AZ::RPI::ModelAsset> m_modelAsset;
        MeshColliderComponent::MeshShapeType m_meshShapeType = MeshColliderComponent::MeshShapeType::ConvexHull;
        Physics::ColliderConfiguration m_configuration;
        bool m_useMeshComponent = true;
    };
}
