#include "ReflectionProbeSSSR.h"

namespace AZ
{
    namespace Render
    {
        ReflectionProbeSSSR::~ReflectionProbeSSSR()
        {
            Shutdown();
        }

        bool ReflectionProbeSSSR::Initialize(uint32_t renderWidth, uint32_t renderHeight)
        {
            m_renderWidth = renderWidth;
            m_renderHeight = renderHeight;
            m_isInitialized = true;
            return true;
        }

        void ReflectionProbeSSSR::Shutdown()
        {
            m_isInitialized = false;
        }

        void ReflectionProbeSSSR::DispatchSSSR(
            RHI::CommandList* commandList,
            RHI::Ptr<RHI::ImageView> colorBuffer,
            RHI::Ptr<RHI::ImageView> depthBuffer,
            RHI::Ptr<RHI::ImageView> normalBuffer,
            RHI::Ptr<RHI::ImageView> roughnessBuffer,
            RHI::Ptr<RHI::ImageView> outputBuffer,
            const Matrix4x4& viewMatrix,
            const Matrix4x4& projMatrix,
            const Matrix4x4& invViewProjMatrix)
        {
            // Placeholder - en un futuro se integrara el compute shader SSSR
            (void)commandList;
            (void)colorBuffer;
            (void)depthBuffer;
            (void)normalBuffer;
            (void)roughnessBuffer;
            (void)outputBuffer;
            (void)viewMatrix;
            (void)projMatrix;
            (void)invViewProjMatrix;
        }
    }
}