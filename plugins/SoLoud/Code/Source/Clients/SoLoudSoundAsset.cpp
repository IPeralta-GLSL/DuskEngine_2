#include <AzCore/Serialization/SerializeContext.h>
#include <SoLoud/SoLoudSoundAsset.h>

namespace SoLoudGem
{
    void SoLoudSoundAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SoLoudSoundAsset, AZ::Data::AssetData>()
                ->Version(1)
                ->Field("Data", &SoLoudSoundAsset::m_data);
        }
    }
}
