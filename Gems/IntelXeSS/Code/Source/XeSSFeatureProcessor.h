#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ::Render
{
    class XeSSFeatureProcessor final
        : public RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(XeSSFeatureProcessor, "{A1B2C3D4-5E6F-7890-ABCD-EF1234567891}", RPI::FeatureProcessor);
        AZ_CLASS_ALLOCATOR(XeSSFeatureProcessor, SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        void Activate() override;
        void Deactivate() override;
        void AddRenderPasses(RPI::RenderPipeline* pipeline) override;
        void Simulate(const SimulatePacket& packet) override;
    };
} // namespace AZ::Render
