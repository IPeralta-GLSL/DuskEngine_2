#include "SoLoudSoundAssetHandler.h"
#include <SoLoud/SoLoudSoundAsset.h>
#include <SoLoud/SoLoudConstants.h>
#include <AzCore/IO/GenericStreams.h>

namespace SoLoudGem
{
    SoLoudSoundAssetHandler::SoLoudSoundAssetHandler()
    {
        Register();
    }

    SoLoudSoundAssetHandler::~SoLoudSoundAssetHandler()
    {
        Unregister();
    }

    void SoLoudSoundAssetHandler::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<SoLoudSoundAsset>::Uuid());
    }

    void SoLoudSoundAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect();
    }

    AZ::Data::AssetPtr SoLoudSoundAssetHandler::CreateAsset(
        [[maybe_unused]] const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        if (type == AZ::AzTypeInfo<SoLoudSoundAsset>::Uuid())
        {
            return aznew SoLoudSoundAsset();
        }
        return nullptr;
    }

    AZ::Data::AssetHandler::LoadResult SoLoudSoundAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        auto* soundAsset = asset.GetAs<SoLoudSoundAsset>();
        if (!soundAsset || !stream)
        {
            return AZ::Data::AssetHandler::LoadResult::Error;
        }

        const size_t dataSize = static_cast<size_t>(stream->GetLength());
        soundAsset->m_data.resize_no_construct(dataSize);
        stream->Read(dataSize, soundAsset->m_data.data());

        return AZ::Data::AssetHandler::LoadResult::LoadComplete;
    }

    void SoLoudSoundAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void SoLoudSoundAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<SoLoudSoundAsset>::Uuid());
    }

    AZ::Data::AssetType SoLoudSoundAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<SoLoudSoundAsset>::Uuid();
    }

    void SoLoudSoundAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back(SoLoudSoundAsset::FileExtension);
    }

    const char* SoLoudSoundAssetHandler::GetAssetTypeDisplayName() const
    {
        return "SoLoud Sound";
    }

    const char* SoLoudSoundAssetHandler::GetBrowserIcon() const
    {
        return "";
    }

    const char* SoLoudSoundAssetHandler::GetGroup() const
    {
        return SoLoudSoundAsset::AssetGroup;
    }

    AZ::Uuid SoLoudSoundAssetHandler::GetComponentTypeId() const
    {
        return AZ::Uuid(SoLoudPlaybackComponentTypeId);
    }

    bool SoLoudSoundAssetHandler::CanCreateComponent(
        [[maybe_unused]] const AZ::Data::AssetId& assetId) const
    {
        return false;
    }
}
