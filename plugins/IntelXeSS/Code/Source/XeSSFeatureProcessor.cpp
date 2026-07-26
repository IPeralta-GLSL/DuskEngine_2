#include <XeSSFeatureProcessor.h>
#include <XeSSPass.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Console/IConsole.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

AZ_CVAR(float, r_xessSharpness, 0.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "XeSS sharpness (0.0 - 1.0)");
AZ_CVAR(int, r_xessQualityMode, static_cast<int>(XESS_QUALITY_SETTING_PERFORMANCE), nullptr, AZ::ConsoleFunctorFlags::Null, "XeSS quality mode");
AZ_CVAR(bool, r_xessEnabled, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Enable XeSS upscaling");

namespace AZ::Render
{
    void XeSSFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<XeSSFeatureProcessor, RPI::FeatureProcessor>()
                ->Version(0);
        }
    }

    void XeSSFeatureProcessor::Activate()
    {
    }

    void XeSSFeatureProcessor::Deactivate()
    {
    }

    void XeSSFeatureProcessor::Simulate(const SimulatePacket& packet)
    {
        auto* scene = GetParentScene();
        if (!scene)
        {
            return;
        }

        for (auto& pipeline : scene->GetRenderPipelines())
        {
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(Name("XeSSPass"), pipeline.get());
            RPI::Pass* foundPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
            if (foundPass)
            {
                auto* xessPass = azrtti_cast<XeSSPass*>(foundPass);
                if (xessPass)
                {
                    foundPass->SetEnabled(r_xessEnabled);
                    xessPass->SetSharpness(r_xessSharpness);
                    xessPass->SetQualityMode(static_cast<xess_quality_settings_t>(static_cast<int>(r_xessQualityMode)));
                }
            }
        }
    }
} // namespace AZ::Render