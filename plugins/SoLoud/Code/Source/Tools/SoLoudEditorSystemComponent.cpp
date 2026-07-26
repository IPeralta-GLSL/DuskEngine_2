#include "SoLoudEditorSystemComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <SoLoud/SoLoudSoundAsset.h>

namespace SoLoudGem
{
    AZ::ComponentDescriptor* SoLoudEditorSystemComponent_CreateDescriptor()
    {
        return SoLoudEditorSystemComponent::CreateDescriptor();
    }

    AZ::TypeId SoLoudEditorSystemComponent_GetTypeId()
    {
        return azrtti_typeid<SoLoudEditorSystemComponent>();
    }

    void SoLoudEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SoLoudEditorSystemComponent, AZ::Component>()->Version(1)->Attribute(
                AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("AssetBuilder") }));
        }
    }

    void SoLoudEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("SoLoudEditorService"));
    }

    void SoLoudEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("SoLoudEditorService"));
    }

    void SoLoudEditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void SoLoudEditorSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetDatabaseService"));
        dependent.push_back(AZ_CRC_CE("AssetCatalogService"));
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void SoLoudEditorSystemComponent::Activate()
    {
        SoLoudSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();

        if (AZ::Data::AssetManager::IsReady() &&
            !AZ::Data::AssetManager::Instance().GetHandler(AZ::AzTypeInfo<SoLoudSoundAsset>::Uuid()))
        {
            auto* soundAssetHandler =
                aznew AzFramework::GenericAssetHandler<SoLoudSoundAsset>(
                    "SoLoud Sound Asset", SoLoudSoundAsset::AssetGroup, SoLoudSoundAsset::FileExtension);
            soundAssetHandler->Register();
            m_editorAssetHandlers.emplace_back(soundAssetHandler);
        }

        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "SoLoud Sound Asset Builder";
        builderDescriptor.m_version = 2;
        builderDescriptor.m_patterns.push_back(
            AssetBuilderSDK::AssetBuilderPattern("*.ogg", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(
            AssetBuilderSDK::AssetBuilderPattern("*.flac", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(
            AssetBuilderSDK::AssetBuilderPattern("*.mp3", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(
            AssetBuilderSDK::AssetBuilderPattern("*.wav", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_busId = azrtti_typeid<SoLoudSoundAssetBuilder>();
        builderDescriptor.m_createJobFunction =
            [this](const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
        {
            m_soundAssetBuilder.CreateJobs(request, response);
        };
        builderDescriptor.m_processJobFunction =
            [this](const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
        {
            m_soundAssetBuilder.ProcessJob(request, response);
        };
        m_soundAssetBuilder.BusConnect(builderDescriptor.m_busId);
        m_builderRegistered = true;
        AssetBuilderSDK::AssetBuilderBus::Broadcast(
            &AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, builderDescriptor);
    }

    void SoLoudEditorSystemComponent::Deactivate()
    {
        if (m_builderRegistered)
        {
            m_soundAssetBuilder.BusDisconnect();
            m_builderRegistered = false;
        }
        m_editorAssetHandlers.clear();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        SoLoudSystemComponent::Deactivate();
    }
}
