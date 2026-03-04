#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ::Render
{
    class Fsr3FeatureProcessor final
        : public RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(Fsr3FeatureProcessor, "{C3D4E5F6-A7B8-9012-CDEF-123456789012}", RPI::FeatureProcessor);
        AZ_CLASS_ALLOCATOR(Fsr3FeatureProcessor, SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        void Activate() override;
        void Deactivate() override;
        void AddRenderPasses(RPI::RenderPipeline* pipeline) override;
        void Simulate(const SimulatePacket& packet) override;
    };
}
