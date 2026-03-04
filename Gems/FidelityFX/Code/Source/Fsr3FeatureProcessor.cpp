#include <Fsr3FeatureProcessor.h>
#include <Fsr3Pass.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Console/IConsole.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

AZ_CVAR(float, r_fsr3Sharpness, 0.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "FSR3 Upscaler sharpness (0.0 - 1.0)");
AZ_CVAR(int, r_fsr3QualityMode, 3, nullptr, AZ::ConsoleFunctorFlags::Null, "FSR3 Upscaler quality mode (0=NativeAA, 1=Quality, 2=Balanced, 3=Performance, 4=UltraPerformance)");

namespace AZ::Render
{
    void Fsr3FeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Fsr3FeatureProcessor, RPI::FeatureProcessor>()
                ->Version(0);
        }
    }

    void Fsr3FeatureProcessor::Activate()
    {
    }

    void Fsr3FeatureProcessor::Deactivate()
    {
    }

    void Fsr3FeatureProcessor::AddRenderPasses([[maybe_unused]] RPI::RenderPipeline* pipeline)
    {
    }

    void Fsr3FeatureProcessor::Simulate([[maybe_unused]] const SimulatePacket& packet)
    {
        auto* scene = GetParentScene();
        if (!scene)
        {
            return;
        }

        for (auto& pipeline : scene->GetRenderPipelines())
        {
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(Name("Fsr3Pass"), pipeline.get());
            RPI::Pass* foundPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
            if (foundPass)
            {
                auto* fsr3Pass = azrtti_cast<Fsr3Pass*>(foundPass);
                if (fsr3Pass)
                {
                    fsr3Pass->SetSharpness(r_fsr3Sharpness);
                    fsr3Pass->SetQualityMode(static_cast<FfxFsr3UpscalerQualityMode>(static_cast<int>(r_fsr3QualityMode)));
                }
            }
        }
    }
}
