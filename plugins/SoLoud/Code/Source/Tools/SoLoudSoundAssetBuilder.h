#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>

namespace SoLoudGem
{
    class SoLoudSoundAssetBuilder
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(SoLoudSoundAssetBuilder, "{A1B2C3D4-E5F6-7A8B-9C0D-1E2F3A4B5C6D}");

        SoLoudSoundAssetBuilder() = default;

        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;
        void ShutDown() override {}
    };
}
