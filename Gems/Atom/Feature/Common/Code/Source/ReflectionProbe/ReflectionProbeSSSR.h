#pragma once
#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        class ReflectionProbeSSSR
        {
        public:
            AZ_CLASS_ALLOCATOR(ReflectionProbeSSSR, AZ::SystemAllocator);
            ReflectionProbeSSSR() = default;
            ~ReflectionProbeSSSR() = default;
            bool Initialize(uint32_t renderWidth, uint32_t renderHeight) { m_renderWidth = renderWidth; m_renderHeight = renderHeight; m_isInitialized = true; return true; }
            void Shutdown() { m_isInitialized = false; }
            void DispatchSSSR() {}
            bool IsEnabled() const { return m_isInitialized; }
        private:
            uint32_t m_renderWidth = 0;
            uint32_t m_renderHeight = 0;
            bool m_isInitialized = false;
        };
    }
}