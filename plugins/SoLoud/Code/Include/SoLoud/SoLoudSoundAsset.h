#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/std/containers/vector.h>

namespace SoLoudGem
{
    class SoLoudSoundAsset : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(SoLoudSoundAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(SoLoudSoundAsset, "{F1E2D3C4-B5A6-7890-FEDC-BA9876543210}", AZ::Data::AssetData);

        static constexpr const char* FileExtension = "soloudaudio";
        static constexpr const char* AssetGroup = "SoLoudSound";
        static constexpr AZ::u32 AssetSubId = 2;

        static void Reflect(AZ::ReflectContext* context);

        SoLoudSoundAsset() = default;
        ~SoLoudSoundAsset() override = default;

        AZStd::vector<AZ::u8> m_data;
    };

    using SoLoudSoundDataAsset = AZ::Data::Asset<SoLoudSoundAsset>;
}
