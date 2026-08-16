#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSlots.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>

namespace JoltPhysics
{
    namespace Pipeline
    {
        class JoltMeshAsset;

        class JoltAssetColliderConfiguration
        {
        public:
            AZ_CLASS_ALLOCATOR(JoltAssetColliderConfiguration, AZ::SystemAllocator);
            AZ_TYPE_INFO(JoltAssetColliderConfiguration, "{8EC9D61B-9180-47C7-87C0-17E13C5A8358}");

            static void Reflect(AZ::ReflectContext* context);
            void UpdateColliderConfiguration(Physics::ColliderConfiguration& colliderConfiguration) const;

            AZStd::optional<AzPhysics::CollisionLayer> m_collisionLayer;
            AZStd::optional<AzPhysics::CollisionGroups::Id> m_collisionGroupId;
            AZStd::optional<bool> m_isTrigger;
            AZStd::optional<AZ::Transform> m_transform;
            AZStd::optional<AZStd::string> m_tag;
        };

        class JoltMeshAssetData
        {
        public:
            AZ_CLASS_ALLOCATOR(JoltMeshAssetData, AZ::SystemAllocator);
            AZ_TYPE_INFO(JoltMeshAssetData, "{4E3329AA-3F48-4942-8D08-DC97AAA9901F}");

            static void Reflect(AZ::ReflectContext* context);

            AZ::Data::Asset<JoltMeshAsset> CreateMeshAsset() const;

            static constexpr AZ::u16 TriangleMeshMaterialIndex = (AZStd::numeric_limits<AZ::u16>::max)();

            using ShapeConfigurationPair = AZStd::pair<AZStd::shared_ptr<JoltAssetColliderConfiguration>,
                AZStd::shared_ptr<Physics::ShapeConfiguration>>;
            using ShapeConfigurationList = AZStd::vector<ShapeConfigurationPair>;

            ShapeConfigurationList m_colliderShapes;
            Physics::MaterialSlots m_materialSlots;
            AZStd::vector<AZ::u16> m_materialIndexPerShape;
        };

        class JoltMeshAsset
            : public AZ::Data::AssetData
        {
        public:
            AZ_CLASS_ALLOCATOR(JoltMeshAsset, AZ::SystemAllocator);
            AZ_RTTI(JoltMeshAsset, "{5D012381-E9E8-4327-8F31-8D3006A1E776}", AZ::Data::AssetData);

            static void Reflect(AZ::ReflectContext* context);

            void SetData(const JoltMeshAssetData& assetData);

            JoltMeshAssetData m_assetData;
        };
    }
}
