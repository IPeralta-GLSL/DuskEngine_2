#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Asset/AssetDataStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SoLoud/SoLoudSoundAsset.h>
#include "SoLoudSoundAssetBuilder.h"

namespace SoLoudGem
{
    void SoLoudSoundAssetBuilder::CreateJobs(
        const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = "SoLoud Sound Asset";
            jobDescriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
            response.m_createJobOutputs.push_back(jobDescriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void SoLoudSoundAssetBuilder::ProcessJob(
        const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        const AZStd::string& fromFile = request.m_fullPath;

        AZ::Data::Asset<SoLoudSoundAsset> soundAsset;
        soundAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        {
            AZ::IO::FileIOStream stream(fromFile.c_str(), AZ::IO::OpenMode::ModeRead);
            if (!AZ::IO::RetryOpenStream(stream))
            {
                AZ_Error("SoLoudSoundAssetBuilder", false, "Source file '%s' could not be opened.", fromFile.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            const AZ::IO::SizeType fileLength = stream.GetLength();
            if (fileLength == 0)
            {
                AZ_Error("SoLoudSoundAssetBuilder", false, "Source file '%s' is empty.", fromFile.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            AZStd::vector<AZ::u8> fileBuffer(fileLength);
            const size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
            if (bytesRead != fileLength)
            {
                AZ_Error("SoLoudSoundAssetBuilder", false, "Source file '%s' could not be read.", fromFile.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            soundAsset->m_data.swap(fileBuffer);
        }

        AZ::Uuid sourceUuid = AZ::Uuid::CreateName(request.m_sourceFile.c_str());
        AZStd::string outputFilename = sourceUuid.ToFixedString().c_str();
        outputFilename += ".";
        outputFilename += SoLoudSoundAsset::FileExtension;

        AZStd::string outputPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), outputFilename.c_str(), outputPath, true);

        if (!AZ::Utils::SaveObjectToFile(outputPath, AZ::DataStream::ST_BINARY, soundAsset.Get()))
        {
            AZ_Error("SoLoudSoundAssetBuilder", false, "Failed to save sound asset to '%s'!", outputPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        AssetBuilderSDK::JobProduct soundJobProduct;
        if (!AssetBuilderSDK::OutputObject(
                soundAsset.Get(), outputPath, azrtti_typeid<SoLoudSoundAsset>(), SoLoudSoundAsset::AssetSubId, soundJobProduct))
        {
            AZ_Error("SoLoudSoundAssetBuilder", false, "Failed to output product dependencies.");
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
        }
        else
        {
            response.m_outputProducts.push_back(AZStd::move(soundJobProduct));
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }
    }
}
