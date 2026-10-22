/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/TickBus.h>

#include <AzFramework/Input/Events/InputTextEventListener.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>

#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/StreamBufferView.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>

#include <Atom/Feature/Base.h>

#include <imgui/imgui.h>

namespace AZ
{
    namespace Render
    {
        //! Custom data for the ImGuiPassData.
        struct ImGuiPassData
            : public RPI::RasterPassData
        {
            AZ_RTTI(ImGuiPassData, "{3E96AF5F-DE1E-4B3B-9833-7164AEAB7C28}", RPI::RasterPassData);
            AZ_CLASS_ALLOCATOR(ImGuiPassData, SystemAllocator);

            ImGuiPassData() = default;
            virtual ~ImGuiPassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<ImGuiPassData, RPI::RasterPassData>()
                        ->Version(1)
                        ->Field("isDefaultImGui", &ImGuiPassData::m_isDefaultImGui)
                        ;
                }
            }

            bool m_isDefaultImGui = false;
        };

        //! This pass owns and manages activation of an Imgui context.
        class ATOM_FEATURE_COMMON_API ImGuiPass
            : public RPI::RenderPass
            , private AzFramework::InputChannelEventListener
            , private AzFramework::InputTextEventListener
        {
            using Base = RPI::RenderPass;
            AZ_RPI_PASS(ImGuiPass);

        public:
            AZ_RTTI(ImGuiPass, "{44EC7CFB-860B-40C8-922D-D54F971E049F}", Base);
            AZ_CLASS_ALLOCATOR(ImGuiPass, SystemAllocator);

            //! Creates a new ImGuiPass
            static RPI::Ptr<ImGuiPass> Create(const RPI::PassDescriptor& descriptor);

            ~ImGuiPass();

            //! Gets the underlying ImGui context.
            ImGuiContext* GetContext();

            //! Allows draw data from other imgui contexts to be rendered on this context.
            void RenderImguiDrawData(const ImDrawData& drawData);

            // AzFramework::InputTextEventListener overrides...
            bool OnInputTextEventFiltered(const AZStd::string& textUTF8) override;

            // AzFramework::InputChannelEventListener overrides...
            bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

        protected:
            explicit ImGuiPass(const RPI::PassDescriptor& descriptor);

            // Pass Behaviour Overrides...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

        private:
            static const int MaxUserTextures = 15;

            //! Class which connects to the tick handler using the tick order required at the start of an ImGui frame.
            class TickHandlerFrameStart : protected TickBus::Handler
            {
            public:
                TickHandlerFrameStart(ImGuiPass& imGuiPass);

            protected:
                // TickBus::Handler overrides...
                int GetTickOrder() override;
                void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;

            private:
                ImGuiPass& m_imGuiPass;
            };

            //! Class which connects to the tick handler using the tick order required at the end of an ImGui frame.
            class TickHandlerFrameEnd : protected TickBus::Handler
            {
            public:
                TickHandlerFrameEnd(ImGuiPass& imGuiPass);

            protected:
                // TickBus::Handler overrides...
                int GetTickOrder() override;
                void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;

            private:
                ImGuiPass& m_imGuiPass;
            };

            struct DrawInfo
            {
                RHI::DrawInstanceArguments m_drawInstanceArgs;
                RHI::GeometryView m_geometryView;
                RHI::Scissor m_scissor;
            };

            // Wrapper around AZStd::vector<DrawInfo> to implement double/triple buffering
            struct BufferedDrawInfos
            {
                static constexpr u32 DrawInfoBuffering = 2;
                AZStd::vector<DrawInfo> m_drawInfos[DrawInfoBuffering];
                u8 m_currentIndex = 0;

                // Move to the next DrawInfo list in a ring buffer fashion
                void NextBuffer() { m_currentIndex = (m_currentIndex + 1) % DrawInfoBuffering; }

                // Get the current DrawInfo list
                AZStd::vector<DrawInfo>& Get() { return m_drawInfos[m_currentIndex]; }
            };

            //! Updates the index and vertex buffers, and returns the total number of draw items.
            uint32_t UpdateImGuiResources();

            // Initializes ImGui related data in the pass. Called during the pass's initialization phase.
            void InitializeImGui();

            // Updates the size of the space the ImGui UI is laid out in. This is the on-screen size
            // of the viewport (the client area of its window), which may be different from the
            // internal render resolution of the viewport.
            void UpdateDisplaySize();

            // Returns the factor that has to be applied to the geometry of the given draw data to
            // convert it from the space it was laid out in to the render target space.
            AZ::Vector2 GetDrawDataScale(const ImDrawData& drawData) const;

            // Builds the font atlas with the glyphs rasterized at the given size and uploads it to
            // the GPU. The size of the UI in the layout space is not affected by it, only the
            // sharpness of the rasterized glyphs.
            void CreateFontAtlas(uint32_t fontSize);

            // Rebuilds the font atlas when the resolution the UI is rendered at changes, so the
            // text keeps the same sharpness as the rest of the image.
            void UpdateFontAtlas();

            // Returns the size the font glyphs have to be rasterized at to match the resolution
            // the UI is rendered at.
            uint32_t GetRequiredFontSize() const;

            ImGuiContext* m_imguiContext = nullptr;
            TickHandlerFrameStart m_tickHandlerFrameStart;
            TickHandlerFrameEnd m_tickHandlerFrameEnd;

            RHI::Ptr<RPI::PipelineStateForDraw> m_pipelineState;
            Data::Instance<RPI::Shader> m_shader;

            Data::Instance<RPI::ShaderResourceGroup> m_resourceGroup;
            RHI::ShaderInputNameIndex m_texturesIndex = "m_textures";
            RHI::ShaderInputNameIndex m_projectionMatrixIndex = "m_projectionMatrix";
            RHI::Viewport m_viewportState;

            RHI::IndexBufferView m_indexBufferView;
            AZStd::array<RHI::StreamBufferView, 2> m_vertexBufferView; // For vertex buffer and instance data

            // List of DrawInfos that can be double or triple buffered
            BufferedDrawInfos m_drawInfos;
            Data::Instance<RPI::StreamingImage> m_fontAtlas;

            AZStd::vector<ImDrawData> m_drawData;
            bool m_isDefaultImGuiPass = false;

            // Whether the pass data indicated that the pass should be used as the default ImGui pass
            bool m_requestedAsDefaultImguiPass = false;

            // ImGui processes mouse wheel movement on NewFrame(), which could be before input events
            // happen, so save the value to apply the most recent value right before NewFrame().
            float m_lastFrameMouseWheel = 0.0;
            
            uint32_t m_viewportWidth = 0;
            uint32_t m_viewportHeight = 0;

            // Size of the space the ImGui UI is laid out in. This matches the on-screen size of the
            // viewport (the client area of its window) so the UI keeps a constant size on screen
            // even when the viewport renders internally at a different resolution, for example when
            // a fixed render height is used.
            uint32_t m_displayWidth = 0;
            uint32_t m_displayHeight = 0;

            // Size (in pixels) the font glyphs are rasterized at in the font atlas. It matches the
            // resolution the text has in the render target so it stays crisp.
            uint32_t m_fontAtlasFontSize = 0;

            // Font size the UI is laid out with, it is independent of the rasterization size.
            static constexpr uint32_t BaseFontSize = 13;

            // Maximum ratio between the rasterization size of the glyphs and the layout font size.
            static constexpr uint32_t MaxFontAtlasScale = 4;

            // Number of frames the required font size has to stay stable before the font atlas is
            // rebuilt, resizing a viewport would otherwise rebuild it on every frame.
            static constexpr uint32_t FontAtlasRebuildDelay = 3;

            // Font size the font atlas is going to be rebuilt with and the number of frames it has
            // been stable for.
            uint32_t m_pendingFontSize = 0;
            uint32_t m_pendingFontSizeFrames = 0;

            AZStd::unordered_map<Data::Instance<RPI::StreamingImage>, uint32_t> m_userTextures;
            Data::Instance<RPI::Buffer> m_instanceBuffer;
            RHI::StreamBufferView m_instanceBufferView;

            // cache the font text id
            void* m_imguiFontTexId = nullptr;

            // Whether the pass has already initialized it's ImGui related data.
            bool m_imguiInitialized = false;
        };
    }   // namespace RPI
}   // namespace AZ
