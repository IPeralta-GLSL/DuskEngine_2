#include <SoLoudModuleInterface.h>

namespace SoLoudGem
{
    extern AZ::TypeId SoLoudEditorSystemComponent_GetTypeId();
    extern AZ::ComponentDescriptor* SoLoudEditorSystemComponent_CreateDescriptor();
    extern AZ::ComponentDescriptor* EditorSoLoudPlaybackComponent_CreateDescriptor();
    extern AZ::ComponentDescriptor* EditorSoLoudListenerComponent_CreateDescriptor();

    class SoLoudEditorModule : public SoLoudModuleInterface
    {
    public:
        AZ_RTTI(SoLoudEditorModule, "{A3B4C5D6-E7F8-9A0B-1C2D-3E4F5A6B7C8D}", SoLoudModuleInterface);
        AZ_CLASS_ALLOCATOR(SoLoudEditorModule, AZ::SystemAllocator, 0);

        SoLoudEditorModule()
        {
            m_descriptors.insert(
                m_descriptors.end(),
                {
                    SoLoudEditorSystemComponent_CreateDescriptor(),
                    EditorSoLoudPlaybackComponent_CreateDescriptor(),
                    EditorSoLoudListenerComponent_CreateDescriptor(),
                });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                SoLoudEditorSystemComponent_GetTypeId(),
            };
        }
    };
}

AZ_DECLARE_MODULE_CLASS(Gem_SoLoud, SoLoudGem::SoLoudEditorModule)
