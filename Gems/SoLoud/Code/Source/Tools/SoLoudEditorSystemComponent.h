#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Clients/SoLoudSystemComponent.h>
#include <Tools/SoLoudSoundAssetBuilder.h>

namespace SoLoudGem
{
    class SoLoudEditorSystemComponent
        : public SoLoudSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = SoLoudSystemComponent;
    public:
        AZ_COMPONENT(SoLoudEditorSystemComponent, "{D4E5F6A7-B8C9-0D1E-2F3A-4B5C6D7E8F9A}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        SoLoudEditorSystemComponent() = default;
        SoLoudEditorSystemComponent(const SoLoudEditorSystemComponent&) = delete;
        SoLoudEditorSystemComponent& operator=(const SoLoudEditorSystemComponent&) = delete;

        void Activate() override;
        void Deactivate() override;

    private:
        SoLoudSoundAssetBuilder m_soundAssetBuilder;
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_editorAssetHandlers;
        bool m_builderRegistered = false;
    };
}
