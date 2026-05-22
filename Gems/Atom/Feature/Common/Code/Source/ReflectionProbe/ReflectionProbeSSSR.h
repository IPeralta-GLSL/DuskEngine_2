#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RHI/CommandList.h>

namespace AZ
{
    namespace Render
    {
        // SSSR (Screen Space Reflections) mediante compute shaders
        // Implementacion basada en ray marching jerarquico (Hi-Z) con denoiser temporal
        class ReflectionProbeSSSR
        {
        public:
            AZ_CLASS_ALLOCATOR(ReflectionProbeSSSR, AZ::SystemAllocator);

            ReflectionProbeSSSR() = default;
            ~ReflectionProbeSSSR();

            // Inicializa los recursos del SSSR
            bool Initialize(uint32_t renderWidth, uint32_t renderHeight);

            // Libera los recursos
            void Shutdown();

            // Ejecuta el SSSR en el CommandList
            void DispatchSSSR(RHI::CommandList* commandList,
                              RHI::Ptr<RHI::ImageView> colorBuffer,
                              RHI::Ptr<RHI::ImageView> depthBuffer,
                              RHI::Ptr<RHI::ImageView> normalBuffer,
                              RHI::Ptr<RHI::ImageView> roughnessBuffer,
                              RHI::Ptr<RHI::ImageView> outputBuffer,
                              const Matrix4x4& viewMatrix,
                              const Matrix4x4& projMatrix,
                              const Matrix4x4& invViewProjMatrix);

            bool IsEnabled() const { return m_isInitialized; }

        private:
            uint32_t m_renderWidth = 0;
            uint32_t m_renderHeight = 0;
            bool m_isInitialized = false;
        };
    }
}