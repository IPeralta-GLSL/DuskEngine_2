#include "EditorSoLoudListenerComponent.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>

namespace SoLoudGem
{
    AZ::ComponentDescriptor* EditorSoLoudListenerComponent_CreateDescriptor()
    {
        return EditorSoLoudListenerComponent::CreateDescriptor();
    }

    void EditorSoLoudListenerComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSoLoudListenerComponent, BaseClass>()->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorSoLoudListenerComponent>("SoLoud Listener", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "SoLoud")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                editContext->Class<SoLoudListenerComponentController>("SoLoudListenerComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SoLoudListenerComponentController::m_config, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                editContext
                    ->Class<SoLoudListenerComponentConfig>("SoLoud Listener Config", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &SoLoudListenerComponentConfig::m_followEntity, "Follow Entity", "")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SoLoudListenerComponentConfig::m_globalVolume, "Global Volume", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f);
            }
        }
    }

    EditorSoLoudListenerComponent::EditorSoLoudListenerComponent(const SoLoudListenerComponentConfig& config)
        : BaseClass(config)
    {
    }

    void EditorSoLoudListenerComponent::Activate()
    {
        BaseClass::Activate();
    }

    void EditorSoLoudListenerComponent::Deactivate()
    {
        BaseClass::Deactivate();
    }

    AZ::u32 EditorSoLoudListenerComponent::OnConfigurationChanged()
    {
        return AZ::Edit::PropertyRefreshLevels::None;
    }
}
