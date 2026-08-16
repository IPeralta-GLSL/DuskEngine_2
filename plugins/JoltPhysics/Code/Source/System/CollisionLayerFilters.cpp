
#include <System/CollisionLayerFilters.h>
#include <System/JoltSystem.h>

namespace JoltPhysics
{
    ObjectLayerPairFilterImpl::ObjectLayerPairFilterImpl()
    {
        if (JoltSystem* system = GetJoltSystem())
        {
            m_collisionGroupMasks = system->GetCollisionMasks();
        }
        else
        {
            AZ_Error("ObjectLayerPairFilterImpl", false, "Failed to set collision masks")
        }
    }

    bool ObjectLayerPairFilterImpl::ShouldCollide(const JPH::ObjectLayer inObject1, const JPH::ObjectLayer inObject2) const
    {
        const AZ::u64 collisionLayer1 = 1ULL << static_cast<AZ::u8>(inObject1 >> 8);
        const AZ::u64 collisionLayer2 = 1ULL << static_cast<AZ::u8>(inObject2 >> 8);

        if (m_collisionGroupMasks != nullptr)
        {
            AZ::u64 collisionMask1 = m_collisionGroupMasks->at(inObject1 >> 16);
            AZ::u64 collisionMask2 = m_collisionGroupMasks->at(inObject2 >> 16);

            const bool collide = collisionMask1 & collisionLayer2 && collisionMask2 & collisionLayer1;

            static int s_pairFilterLogCount = 0;
            if (s_pairFilterLogCount++ < 40)
            {
                AZ_TracePrintf("JoltFilter", "PairFilter obj1=0x%08x obj2=0x%08x mask1=%llx mask2=%llx layer1=%llx layer2=%llx -> %s",
                    inObject1, inObject2, collisionMask1, collisionMask2, collisionLayer1, collisionLayer2,
                    collide ? "COLLIDE" : "NO");
            }

            return collide;
        }
        AZ_Warning("ObjectLayerPairFilterImpl", false, "JoltSystem pointer was null.")
        return false;
    }
}
