#include "SoLoudSystemComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <SoLoud/SoLoudSoundAsset.h>
#include "SoLoudSoundAssetHandler.h"

#include <soloud.h>

namespace SoLoudGem
{
    AZ::ComponentDescriptor* SoLoudSystemComponent_CreateDescriptor()
    {
        return SoLoudSystemComponent::CreateDescriptor();
    }

    AZ::TypeId SoLoudSystemComponent_GetTypeId()
    {
        return azrtti_typeid<SoLoudSystemComponent>();
    }

    void SoLoudSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        SoLoudSoundAsset::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SoLoudSystemComponent, AZ::Component>()->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SoLoudSystemComponent>("SoLoud Audio", "SoLoud audio engine integration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void SoLoudSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SoLoudService"));
    }

    void SoLoudSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("SoLoudService"));
    }

    void SoLoudSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void SoLoudSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    SoLoudSystemComponent::SoLoudSystemComponent()
    {
        if (SoLoudInterface::Get() == nullptr)
        {
            SoLoudInterface::Register(this);
        }
    }

    SoLoudSystemComponent::~SoLoudSystemComponent()
    {
        if (SoLoudInterface::Get() == this)
        {
            SoLoudInterface::Unregister(this);
        }
    }

    void SoLoudSystemComponent::Init()
    {
    }

    void SoLoudSystemComponent::Activate()
    {
        m_soloud = AZStd::make_unique<SoLoud::Soloud>();
        SoLoud::result r = m_soloud->init();
        if (r != SoLoud::SO_NO_ERROR)
        {
            AZ_Error("SoLoud", false, "Failed to initialize SoLoud engine, error %d", r);
            m_soloud.reset();
            return;
        }

        m_initialized = true;
        SoLoudRequestBus::Handler::BusConnect();

        if (!AZ::Data::AssetManager::IsReady())
        {
            return;
        }

        if (!AZ::Data::AssetManager::Instance().GetHandler(AZ::AzTypeInfo<SoLoudSoundAsset>::Uuid()))
        {
            SoLoudSoundAssetHandler* handler = aznew SoLoudSoundAssetHandler();
            AZ::Data::AssetCatalogRequestBus::Broadcast(
                &AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, AZ::AzTypeInfo<SoLoudSoundAsset>::Uuid());
            AZ::Data::AssetCatalogRequestBus::Broadcast(
                &AZ::Data::AssetCatalogRequests::AddExtension, SoLoudSoundAsset::FileExtension);
            m_assetHandlers.emplace_back(handler);
        }
    }

    void SoLoudSystemComponent::Deactivate()
    {
        SoLoudRequestBus::Handler::BusDisconnect();

        m_assetHandlers.clear();

        if (m_initialized && m_soloud)
        {
            m_soloud->stopAll();
            m_soloud->deinit();
            m_soloud.reset();
        }
        m_initialized = false;
    }

    SoLoud::Soloud* SoLoudSystemComponent::GetEngine()
    {
        return m_soloud.get();
    }

    void SoLoudSystemComponent::SetGlobalVolume(float volume)
    {
        m_globalVolume = volume;
        if (m_soloud)
        {
            m_soloud->setGlobalVolume(m_globalVolume);
        }
    }

    float SoLoudSystemComponent::GetGlobalVolume() const
    {
        return m_globalVolume;
    }
}
