#include <JoltPhysics/Pipeline/JoltMeshAsset.h>
#include <Pipeline/JoltMeshAssetHandler.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

namespace JoltPhysics
{
    namespace Pipeline
    {
        const char* JoltMeshAssetHandler::s_assetFileExtension = "joltmesh";

        JoltMeshAssetHandler::~JoltMeshAssetHandler()
        {
            Unregister();
        }

        void JoltMeshAssetHandler::Register()
        {
            if (m_registered)
            {
                return;
            }

            const bool assetManagerReady = AZ::Data::AssetManager::IsReady();
            AZ_Error("Jolt Mesh Asset", assetManagerReady, "Asset manager isn't ready.");
            if (assetManagerReady)
            {
                AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<JoltMeshAsset>::Uuid());
            }

            AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<JoltMeshAsset>::Uuid());
            m_registered = true;
        }

        void JoltMeshAssetHandler::Unregister()
        {
            if (!m_registered)
            {
                return;
            }

            AZ::AssetTypeInfoBus::Handler::BusDisconnect();

            if (AZ::Data::AssetManager::IsReady())
            {
                AZ::Data::AssetManager::Instance().UnregisterHandler(this);
            }

            m_registered = false;
        }

        AZ::Data::AssetType JoltMeshAssetHandler::GetAssetType() const
        {
            return AZ::AzTypeInfo<JoltMeshAsset>::Uuid();
        }

        void JoltMeshAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
        {
            extensions.push_back(s_assetFileExtension);
        }

        const char* JoltMeshAssetHandler::GetAssetTypeDisplayName() const
        {
            return "Jolt Collision Mesh";
        }

        const char* JoltMeshAssetHandler::GetBrowserIcon() const
        {
            return "";
        }

        const char* JoltMeshAssetHandler::GetGroup() const
        {
            return "Physics";
        }

        AZ::Uuid JoltMeshAssetHandler::GetComponentTypeId() const
        {
            return AZ::Uuid::CreateNull();
        }

        bool JoltMeshAssetHandler::CanCreateComponent([[maybe_unused]] const AZ::Data::AssetId& assetId) const
        {
            return false;
        }

        AZ::Data::AssetPtr JoltMeshAssetHandler::CreateAsset([[maybe_unused]] const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
        {
            if (type == AZ::AzTypeInfo<JoltMeshAsset>::Uuid())
            {
                return aznew JoltMeshAsset();
            }

            AZ_Error("Jolt Mesh Asset", false, "This handler deals only with JoltMeshAsset type.");
            return nullptr;
        }

        template<typename AssetType>
        static bool FixUpAssetIdByHint(AZ::Data::Asset<AssetType>& asset)
        {
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, asset.GetHint().c_str(),
                AZ::Data::s_invalidAssetType, false);

            if (assetId.IsValid())
            {
                asset.Create(assetId, false);
                return true;
            }
            return false;
        }

        AZ::Data::AssetHandler::LoadResult JoltMeshAssetHandler::LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            JoltMeshAsset* meshAsset = asset.GetAs<JoltMeshAsset>();
            if (!meshAsset)
            {
                AZ_Error("Jolt Mesh Asset", false, "This should be a JoltMeshAsset, as this is the only type we process.");
                return AZ::Data::AssetHandler::LoadResult::Error;
            }

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            if (!AZ::Utils::LoadObjectFromStreamInPlace(
                *stream, meshAsset->m_assetData, serializeContext, AZ::ObjectStream::FilterDescriptor(assetLoadFilterCB)))
            {
                return AZ::Data::AssetHandler::LoadResult::Error;
            }

            for (size_t slotId = 0; slotId < meshAsset->m_assetData.m_materialSlots.GetSlotsCount(); ++slotId)
            {
                AZ::Data::Asset<Physics::MaterialAsset> materialAsset = meshAsset->m_assetData.m_materialSlots.GetMaterialAsset(slotId);
                if (!materialAsset.GetId().IsValid() && !materialAsset.GetHint().empty())
                {
                    if (FixUpAssetIdByHint(materialAsset))
                    {
                        meshAsset->m_assetData.m_materialSlots.SetMaterialAsset(slotId, materialAsset);
                    }
                    else
                    {
                        AZ_Warning("Jolt Mesh Asset", false,
                            "Loading Jolt Mesh '%s' it didn't find physics material '%s', assigned to slot '%.*s'. Default physics material will be used.",
                            asset.GetHint().c_str(),
                            materialAsset.GetHint().c_str(),
                            AZ_STRING_ARG(meshAsset->m_assetData.m_materialSlots.GetSlotName(slotId)));
                    }
                }
            }

            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }

        void JoltMeshAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
        {
            delete ptr;
        }

        void JoltMeshAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
        {
            assetTypes.push_back(AZ::AzTypeInfo<JoltMeshAsset>::Uuid());
        }
    }
}
