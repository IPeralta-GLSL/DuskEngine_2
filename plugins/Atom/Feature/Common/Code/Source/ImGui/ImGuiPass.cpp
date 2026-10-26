/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/ImGui/ImGuiPass.h>

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Windowing/WindowBus.h>

#include <cmath>

#include <Atom/RHI/CommandList.h>

#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>

#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>

#include <Atom_Feature_Traits_Platform.h>
#include <Atom/Feature/ImGui/SystemBus.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            [[maybe_unused]] static const char* PassName = "ImGuiPass";
            static const char* ImguiShaderFilePath = "Shaders/imgui/imgui.azshader";
        }

        class ImguiContextScope
        {
        public:
            explicit ImguiContextScope(ImGuiContext* newContext = nullptr)
                : m_savedContext(ImGui::GetCurrentContext())
            {
                ImGui::SetCurrentContext(newContext);
            }

            ~ImguiContextScope()
            {
                ImGui::SetCurrentContext(m_savedContext);
            }

        private:
            ImGuiContext* m_savedContext = nullptr;
        };

        RPI::Ptr<Render::ImGuiPass> ImGuiPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew ImGuiPass(descriptor);
        }

        ImGuiPass::ImGuiPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
            , AzFramework::InputChannelEventListener(AzFramework::InputChannelEventListener::GetPriorityDebugUI() - 1) // Give ImGui manager priority over the pass
            , AzFramework::InputTextEventListener(AzFramework::InputTextEventListener::GetPriorityDebugUI() - 1) // Give ImGui manager priority over the pass
            , m_tickHandlerFrameStart(*this)
            , m_tickHandlerFrameEnd(*this)
        {
            const ImGuiPassData* imguiPassData = RPI::PassUtils::GetPassData<ImGuiPassData>(descriptor);
            m_requestedAsDefaultImguiPass = imguiPassData != nullptr ? imguiPassData->m_isDefaultImGui : false;
        }

        ImGuiPass::~ImGuiPass()
        {
            if (!m_imguiInitialized)
            {
                return;
            }

            if (m_isDefaultImGuiPass)
            {
                ImGuiSystemRequestBus::Broadcast(&ImGuiSystemRequestBus::Events::RemoveDefaultImGuiPass, this);
            }

            ImGuiContext* contextToRestore = ImGui::GetCurrentContext();
            if (contextToRestore == m_imguiContext)
            {
                contextToRestore = nullptr; // Don't restore this context since it's being deleted.
            }

            ImGui::SetCurrentContext(m_imguiContext);
            ImGui::DestroyContext(m_imguiContext);
            m_imguiContext = nullptr;
            ImGui::SetCurrentContext(contextToRestore);

            AzFramework::InputTextEventListener::BusDisconnect();
            AzFramework::InputChannelEventListener::BusDisconnect();
        }

        ImGuiContext* ImGuiPass::GetContext()
        {
            return m_imguiContext;
        }

        void ImGuiPass::RenderImguiDrawData(const ImDrawData& drawData)
        {
            m_drawData.push_back(drawData);
        }

        ImGuiPass::TickHandlerFrameStart::TickHandlerFrameStart(ImGuiPass& imGuiPass)
            : m_imGuiPass(imGuiPass)
        {
            TickBus::Handler::BusConnect();
        }

        int ImGuiPass::TickHandlerFrameStart::GetTickOrder()
        {
            return AZ::ComponentTickBus::TICK_PRE_RENDER;
        }

        void ImGuiPass::TickHandlerFrameStart::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
        {
            auto imguiContextScope = ImguiContextScope(m_imGuiPass.m_imguiContext);

            // The font atlas has to match the resolution the UI is rendered at, it is rebuilt before
            // the frame starts so the layout and the rasterized glyphs are always in sync.
            m_imGuiPass.UpdateFontAtlas();

            ImGui::NewFrame();

            // Let listeners draw into this pass's context before the frame is rendered.
            ImGuiSystemNotificationBus::Broadcast(&ImGuiSystemNotifications::OnImGuiPassFrame, &m_imGuiPass);

            auto& io = ImGui::GetIO();
            io.DeltaTime = deltaTime;
        }

        ImGuiPass::TickHandlerFrameEnd::TickHandlerFrameEnd(ImGuiPass& imGuiPass)
            : m_imGuiPass(imGuiPass)
        {
            TickBus::Handler::BusConnect();
        }

        int ImGuiPass::TickHandlerFrameEnd::GetTickOrder()
        {
            // ImGui::NewFrame() must be called (see ImGuiPass::TickHandlerFrameStart::OnTick) after populating
            // ImGui::GetIO().NavInputs (see ImGuiPass::OnInputChannelEventFiltered), and paired with a call to
            // ImGui::EndFrame() (see ImGuiPass::TickHandlerFrameEnd::OnTick); if this is not called explicitly
            // then it will be called from inside ImGui::Render() (see ImGuiPass::SetupFrameGraphDependencies).
            //
            // ImGui::Render() gets called (indirectly) from OnSystemTick, so we cannot rely on it being paired
            // with a matching call to ImGui::NewFrame() that gets called from OnTick, because OnSystemTick and
            // OnTick can be called at different frequencies under some circumstances (namely from the editor).
            //
            // To account for this we must explicitly call ImGui::EndFrame() once a frame from OnTick to ensure
            // that every call to ImGui::NewFrame() has been matched with a call to ImGui::EndFrame(), but only
            // after ImGui::Render() has had the chance first (if so calling ImGui::EndFrame() again is benign).
            //
            // Because ImGui::Render() gets called (indirectly) from OnSystemTick, which usually happens at the
            // start of every frame, we give TickHandlerFrameEnd::OnTick() the order of TICK_FIRST such that it
            // will be called first on the regular tick bus, which is invoked immediately after the system tick.
            //
            // So while returning TICK_FIRST is incredibly counter-intuitive, hopefully that all explains why.
            return AZ::ComponentTickBus::TICK_FIRST;
        }

        void ImGuiPass::TickHandlerFrameEnd::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
        {
            auto imguiContextScope = ImguiContextScope(m_imGuiPass.m_imguiContext);
            ImGui::EndFrame();
        }

        bool ImGuiPass::OnInputTextEventFiltered(const AZStd::string& textUTF8)
        {
            auto imguiContextScope = ImguiContextScope(m_imguiContext);
            auto& io = ImGui::GetIO();
            io.AddInputCharactersUTF8(textUTF8.c_str());
            return io.WantTextInput;
        }

        bool ImGuiPass::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
        {
            if (!IsEnabled() || GetRenderPipeline() == nullptr || GetRenderPipeline()->GetScene() == nullptr)
            {
                return false;
            }

            auto imguiContextScope = ImguiContextScope(m_imguiContext);

            auto& io = ImGui::GetIO();

            bool shouldCaptureEvent = false;

            // Matches ImGuiKey_ enum
            static const AzFramework::InputChannelId ImGuiKeyChannels[] =
            {
                AzFramework::InputDeviceKeyboard::Key::EditTab, // ImGuiKey_Tab
                AzFramework::InputDeviceKeyboard::Key::NavigationArrowLeft, // ImGuiKey_LeftArrow
                AzFramework::InputDeviceKeyboard::Key::NavigationArrowRight, // ImGuiKey_RightArrow
                AzFramework::InputDeviceKeyboard::Key::NavigationArrowUp, // ImGuiKey_UpArrow
                AzFramework::InputDeviceKeyboard::Key::NavigationArrowDown, // ImGuiKey_DownArrow
                AzFramework::InputDeviceKeyboard::Key::NavigationPageUp, // ImGuiKey_PageUp
                AzFramework::InputDeviceKeyboard::Key::NavigationPageDown, // ImGuiKey_PageDown
                AzFramework::InputDeviceKeyboard::Key::NavigationHome, // ImGuiKey_Home
                AzFramework::InputDeviceKeyboard::Key::NavigationEnd, // ImGuiKey_End
                AzFramework::InputDeviceKeyboard::Key::NavigationInsert, // ImGuiKey_Insert
                AzFramework::InputDeviceKeyboard::Key::NavigationDelete, // ImGuiKey_Delete
                AzFramework::InputDeviceKeyboard::Key::EditBackspace, // ImGuiKey_Backspace
                AzFramework::InputDeviceKeyboard::Key::EditSpace, // ImGuiKey_Space
                AzFramework::InputDeviceKeyboard::Key::EditEnter, // ImGuiKey_Enter
                AzFramework::InputDeviceKeyboard::Key::Escape, // ImGuiKey_Escape
                AzFramework::InputDeviceKeyboard::Key::NumPadEnter, // ImGuiKey_KeyPadEnter
                AzFramework::InputDeviceKeyboard::Key::AlphanumericA, // ImGuiKey_A
                AzFramework::InputDeviceKeyboard::Key::AlphanumericC, // ImGuiKey_C
                AzFramework::InputDeviceKeyboard::Key::AlphanumericV, // ImGuiKey_V
                AzFramework::InputDeviceKeyboard::Key::AlphanumericX, // ImGuiKey_X
                AzFramework::InputDeviceKeyboard::Key::AlphanumericY, // ImGuiKey_Y
                AzFramework::InputDeviceKeyboard::Key::AlphanumericZ  // ImGuiKey_Z
            };

            static_assert(AZ_ARRAY_SIZE(ImGuiKeyChannels) == ImGuiKey_COUNT, "ImGui key input enum does not match input channels array.");

            // Matches ImGuiNavInput_ enum
            static const AzFramework::InputChannelId ImGuiNavChannels[] =
            {
                AzFramework::InputDeviceGamepad::Button::A, // ImGuiNavInput_Activate
                AzFramework::InputDeviceGamepad::Button::B, // ImGuiNavInput_Cancel
                AzFramework::InputDeviceGamepad::Button::Y, // ImGuiNavInput_Input
                AzFramework::InputDeviceGamepad::Button::X, // ImGuiNavInput_Menu
                AzFramework::InputDeviceGamepad::Button::DL, // ImGuiNavInput_DpadLeft
                AzFramework::InputDeviceGamepad::Button::DR, // ImGuiNavInput_DpadRight
                AzFramework::InputDeviceGamepad::Button::DU, // ImGuiNavInput_DpadUp
                AzFramework::InputDeviceGamepad::Button::DD, // ImGuiNavInput_DpadDown
                AzFramework::InputDeviceGamepad::ThumbStickDirection::LL, // ImGuiNavInput_LStickLeft
                AzFramework::InputDeviceGamepad::ThumbStickDirection::LR, // ImGuiNavInput_LStickRight
                AzFramework::InputDeviceGamepad::ThumbStickDirection::LU, // ImGuiNavInput_LStickUp
                AzFramework::InputDeviceGamepad::ThumbStickDirection::LD, // ImGuiNavInput_LStickDown
                AzFramework::InputDeviceGamepad::Button::L1, // ImGuiNavInput_FocusPrev
                AzFramework::InputDeviceGamepad::Button::R1, // ImGuiNavInput_FocusNext
                AzFramework::InputDeviceGamepad::Trigger::L2, // ImGuiNavInput_TweakSlow
                AzFramework::InputDeviceGamepad::Trigger::R2, // ImGuiNavInput_TweakFast
            };

            static_assert(AZ_ARRAY_SIZE(ImGuiNavChannels) == (ImGuiNavInput_InternalStart_), "ImGui nav input enum does not match input channels array.");

            const AzFramework::InputChannelId& inputChannelId = inputChannel.GetInputChannelId();
            switch (inputChannel.GetState())
            {
            case AzFramework::InputChannel::State::Began:
            case AzFramework::InputChannel::State::Updated: // update the camera rotation
            {
                /// Mouse Events
                if (inputChannelId == AzFramework::InputDeviceMouse::SystemCursorPosition)
                {
                    const auto* position = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();

                    AZ_Assert(position, "Expected positiondata2d but found nullptr");

                    io.MousePos.x = position->m_normalizedPosition.GetX() * static_cast<float>(io.DisplaySize.x);
                    io.MousePos.y = position->m_normalizedPosition.GetY() * static_cast<float>(io.DisplaySize.y);
                    shouldCaptureEvent = io.WantCaptureMouse;
                }

                if (inputChannelId == AzFramework::InputDeviceMouse::Button::Left ||
                    inputChannelId == AzFramework::InputDeviceTouch::Touch::Index0)
                {
                    io.MouseDown[0] = true;
                    const auto* position = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();
                    AZ_Assert(position, "Expected positiondata2d but found nullptr");
                    io.MousePos.x = position->m_normalizedPosition.GetX() * static_cast<float>(io.DisplaySize.x);
                    io.MousePos.y = position->m_normalizedPosition.GetY() * static_cast<float>(io.DisplaySize.y);
                    shouldCaptureEvent = io.WantCaptureMouse;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Button::Right)
                {
                    io.MouseDown[1] = true;
                    shouldCaptureEvent = io.WantCaptureMouse;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Button::Middle)
                {
                    io.MouseDown[2] = true;
                    shouldCaptureEvent = io.WantCaptureMouse;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Movement::Z)
                {
                    const float MouseWheelDeltaScale = 1.0f / 120.0f; // based on WHEEL_DELTA in WinUser.h
                    m_lastFrameMouseWheel += inputChannel.GetValue() * MouseWheelDeltaScale;
                    shouldCaptureEvent = io.WantCaptureMouse;
                }

                /// Keyboard Modifiers
                else if (
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierShiftL ||
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierShiftR)
                {
                    io.KeyShift = true;
                }
                else if (
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierAltL ||
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierAltR)
                {
                    io.KeyAlt = true;
                }
                else if (
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierCtrlL ||
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierCtrlR)
                {
                    io.KeyCtrl = true;
                }

                /// Specific Key & Gamepad Events
                else
                {
                    for (size_t i = 0; i < AZ_ARRAY_SIZE(ImGuiKeyChannels); ++i)
                    {
                        if (inputChannelId == ImGuiKeyChannels[i])
                        {
                            io.KeysDown[i] = true;
                            shouldCaptureEvent = io.WantCaptureKeyboard;
                            break;
                        }
                    }

                    for (size_t i = 0; i < AZ_ARRAY_SIZE(ImGuiNavChannels); ++i)
                    {
                        if (inputChannelId == ImGuiNavChannels[i])
                        {
                            io.NavInputs[i] = true;
                            break;
                        }
                    }
                }
                break;
            }

            case AzFramework::InputChannel::State::Ended:
            {
                /// Mouse Events
                if (inputChannelId == AzFramework::InputDeviceMouse::Button::Left ||
                    inputChannelId == AzFramework::InputDeviceTouch::Touch::Index0)
                {
                    io.MouseDown[0] = false;
                    const auto* position = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();
                    AZ_Assert(position, "Expected positiondata2d but found nullptr");
                    io.MousePos.x = position->m_normalizedPosition.GetX() * static_cast<float>(io.DisplaySize.x);
                    io.MousePos.y = position->m_normalizedPosition.GetY() * static_cast<float>(io.DisplaySize.y);
                    shouldCaptureEvent = io.WantCaptureMouse;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Button::Right)
                {
                    io.MouseDown[1] = false;
                    shouldCaptureEvent = io.WantCaptureMouse;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Button::Middle)
                {
                    io.MouseDown[2] = false;
                    shouldCaptureEvent = io.WantCaptureMouse;
                }

                /// Keyboard Modifiers
                else if (
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierShiftL ||
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierShiftR)
                {
                    io.KeyShift = false;
                }
                else if (
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierAltL ||
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierAltR)
                {
                    io.KeyAlt = false;
                }
                else if (
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierCtrlL ||
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierCtrlR)
                {
                    io.KeyCtrl = false;
                }

                /// Specific Key & Gamepad Events
                else
                {
                    for (size_t i = 0; i < AZ_ARRAY_SIZE(ImGuiKeyChannels); ++i)
                    {
                        if (inputChannelId == ImGuiKeyChannels[i])
                        {
                            io.KeysDown[i] = false;
                            shouldCaptureEvent = io.WantCaptureKeyboard;
                            break;
                        }
                    }

                    for (size_t i = 0; i < AZ_ARRAY_SIZE(ImGuiNavChannels); ++i)
                    {
                        if (inputChannelId == ImGuiKeyChannels[i])
                        {
                            io.NavInputs[i] = false;
                            break;
                        }
                    }
                }
                break;
            }
            case AzFramework::InputChannel::State::Idle:
                break;
            }

            return shouldCaptureEvent;
        }

        void ImGuiPass::InitializeImGui()
        {
            if (m_requestedAsDefaultImguiPass)
            {
                m_isDefaultImGuiPass = true;
                ImGuiSystemRequestBus::Broadcast(&ImGuiSystemRequestBus::Events::PushDefaultImGuiPass, this);
            }

            // This ImguiContextScope is just to ensure we set the imgui context to what it was previously at the end of this function
            // The nullptr param is irrelevant as the imgui context gets set to the new context in ImGui::CreateContext
            auto imguiContextScope = ImguiContextScope(nullptr);
            m_imguiContext = ImGui::CreateContext();
            ImGui::StyleColorsDark();

            auto& io = ImGui::GetIO();
#if defined(AZ_TRAIT_IMGUI_INI_FILENAME)
            io.IniFilename = AZ_TRAIT_IMGUI_INI_FILENAME;
#endif

            // ImGui IO Setup
            {
                for (size_t i = 0; i < ImGuiKey_COUNT; ++i)
                {
                    io.KeyMap[static_cast<ImGuiKey_>(i)] = static_cast<int>(i);
                }

                // Touch input
                const AzFramework::InputDevice* inputDevice = nullptr;
                AzFramework::InputDeviceRequestBus::EventResult(inputDevice,
                    AzFramework::InputDeviceTouch::Id,
                    &AzFramework::InputDeviceRequests::GetInputDevice);

                if (inputDevice && inputDevice->IsSupported())
                {
                    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
                }

                // Gamepad input
                inputDevice = nullptr;
                AzFramework::InputDeviceRequestBus::EventResult(inputDevice,
                    AzFramework::InputDeviceGamepad::IdForIndex0,
                    &AzFramework::InputDeviceRequests::GetInputDevice);
                if (inputDevice && inputDevice->IsSupported())
                {
                    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
                    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
                }

                // Set initial display size to something reasonable (this will be updated in FramePrepare)
                io.DisplaySize.x = 1920;
                io.DisplaySize.y = 1080;
            }

            {
                m_shader = RPI::LoadCriticalShader(ImguiShaderFilePath);

                m_pipelineState = aznew RPI::PipelineStateForDraw;
                m_pipelineState->Init(m_shader);

                RHI::InputStreamLayoutBuilder layoutBuilder;
                layoutBuilder.AddBuffer()
                    ->Channel("POSITION", RHI::Format::R32G32_FLOAT)
                    ->Channel("UV", RHI::Format::R32G32_FLOAT)
                    ->Channel("COLOR", RHI::Format::R8G8B8A8_UNORM);
                layoutBuilder.AddBuffer(RHI::StreamStepFunction::PerInstance)
                    ->Channel("INSTANCE_DATA", RHI::Format::R32_UINT);
                m_pipelineState->InputStreamLayout() = layoutBuilder.End();

            }

            // Get shader resource group
            {
                auto perPassSrgLayout = m_shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Pass);
                if (!perPassSrgLayout)
                {
                    AZ_Error(PassName, false, "Failed to get shader resource group layout");
                    return;
                }

                m_resourceGroup = RPI::ShaderResourceGroup::Create(m_shader->GetAsset(), m_shader->GetSupervariantIndex(), perPassSrgLayout->GetName());
                if (!m_resourceGroup)
                {
                    AZ_Error(PassName, false, "Failed to create shader resource group");
                    return;
                }
            }

            // Build the font atlas and upload it to the GPU.
            CreateFontAtlas(BaseFontSize);

            // ImGuiPass will support binding 16 textures at most per frame. 
            const uint32_t instanceData[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
            RPI::CommonBufferDescriptor desc;
            desc.m_poolType = RPI::CommonBufferPoolType::StaticInputAssembly;
            desc.m_bufferName = "InstanceBuffer";
            desc.m_elementSize = 4;
            desc.m_byteCount = 64;
            desc.m_bufferData = instanceData;
            m_instanceBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
            m_instanceBufferView = RHI::StreamBufferView(
                *m_instanceBuffer->GetRHIBuffer(),
                0,
                aznumeric_cast<uint32_t>(desc.m_byteCount),
                aznumeric_cast<uint32_t>(desc.m_elementSize));

            ImGui::NewFrame();
            AzFramework::InputChannelEventListener::Connect();
            AzFramework::InputTextEventListener::Connect();
        }

        void ImGuiPass::InitializeInternal()
        {
            Base::InitializeInternal();
            if (!m_imguiInitialized)
            {
                InitializeImGui();
                m_imguiInitialized = true;
            }

            // Set output format and finalize pipeline state
            m_pipelineState->SetOutputFromPass(this);
            m_pipelineState->Finalize();
        }

        void ImGuiPass::FrameBeginInternal(FramePrepareParams params)
        {
            auto imguiContextScope = ImguiContextScope(m_imguiContext);

            m_viewportWidth = static_cast<uint32_t>(params.m_viewportState.m_maxX - params.m_viewportState.m_minX);
            m_viewportHeight = static_cast<uint32_t>(params.m_viewportState.m_maxY - params.m_viewportState.m_minY);

            // The ImGui UI is laid out in the on-screen space of the viewport instead of the
            // internal render resolution. This keeps the UI at a constant size on screen when the
            // viewport renders internally at a different resolution, for example when a fixed
            // render height is used. The resulting geometry is scaled to the render target when
            // the draw data is uploaded.
            UpdateDisplaySize();

            auto& io = ImGui::GetIO();
            io.DisplaySize.x = AZStd::max<float>(1.0f, static_cast<float>(m_displayWidth));
            io.DisplaySize.y = AZStd::max<float>(1.0f, static_cast<float>(m_displayHeight));
            io.DisplayFramebufferScale = ImVec2(
                static_cast<float>(m_viewportWidth) / AZStd::max<float>(1.0f, static_cast<float>(m_displayWidth)),
                static_cast<float>(m_viewportHeight) / AZStd::max<float>(1.0f, static_cast<float>(m_displayHeight)));

            Matrix4x4 projectionMatrix =
                Matrix4x4::CreateFromRows(
                    AZ::Vector4(2.0f / m_viewportWidth, 0.0f, 0.0f, -1.0f),
                    AZ::Vector4(0.0f, -2.0f / m_viewportHeight, 0.0f, 1.0f),
                    AZ::Vector4(0.0f, 0.0f, 0.5f, 0.5f),
                    AZ::Vector4(0.0f, 0.0f, 0.0f, 1.0f));

            m_resourceGroup->SetConstant(m_projectionMatrixIndex, projectionMatrix);

            m_viewportState = params.m_viewportState;

            Base::FrameBeginInternal(params);
        }

        void ImGuiPass::UpdateDisplaySize()
        {
            AzFramework::WindowSize clientSize;

            if (const auto* pipeline = GetRenderPipeline())
            {
                if (const auto windowHandle = pipeline->GetWindowHandle())
                {
                    AzFramework::WindowRequestBus::EventResult(
                        clientSize, windowHandle, &AzFramework::WindowRequestBus::Events::GetClientAreaSize);
                }
            }

            // Fall back to the render resolution when the on-screen size is not available, in that
            // case both spaces are the same and no scaling is required.
            m_displayWidth = (clientSize.m_width > 0) ? clientSize.m_width : m_viewportWidth;
            m_displayHeight = (clientSize.m_height > 0) ? clientSize.m_height : m_viewportHeight;
        }

        AZ::Vector2 ImGuiPass::GetDrawDataScale(const ImDrawData& drawData) const
        {
            // The geometry of the draw data is expressed in the space it was laid out in, which is
            // not necessarily the render resolution of the viewport. Convert it to the render
            // target space.
            const float scaleX = drawData.DisplaySize.x > 0.0f
                ? static_cast<float>(m_viewportWidth) / drawData.DisplaySize.x
                : 1.0f;
            const float scaleY = drawData.DisplaySize.y > 0.0f
                ? static_cast<float>(m_viewportHeight) / drawData.DisplaySize.y
                : 1.0f;

            return AZ::Vector2(scaleX, scaleY);
        }

        void ImGuiPass::CreateFontAtlas(uint32_t fontSize)
        {
            auto imguiContextScope = ImguiContextScope(m_imguiContext);
            auto& io = ImGui::GetIO();

            // Rasterize the glyphs at the size the text has in the render target, this keeps the
            // text as sharp as the rest of the image when the viewport renders internally at a
            // higher resolution than its on-screen size.
            ImFontConfig fontConfig;
            fontConfig.SizePixels = static_cast<float>(fontSize);

            io.Fonts->Clear();
            io.Fonts->AddFontDefault(&fontConfig);
            io.Fonts->Build();

            // The size of the UI in the layout space is kept constant, only the resolution of the
            // rasterized glyphs changes.
            io.FontGlobalScale = static_cast<float>(BaseFontSize) / static_cast<float>(fontSize);

            uint8_t* pixels = nullptr;
            int32_t width = 0;
            int32_t height = 0;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

            const uint32_t pixelDataSize = static_cast<uint32_t>(width) * static_cast<uint32_t>(height) * 4;

            RHI::Size imageSize;
            imageSize.m_width = aznumeric_cast<uint32_t>(width);
            imageSize.m_height = aznumeric_cast<uint32_t>(height);

            Data::Instance<RPI::StreamingImagePool> streamingImagePool = RPI::ImageSystemInterface::Get()->GetSystemStreamingPool();

            // CreateFromCpuData will add the image to the instance database. Every pass owns its own
            // atlas since the rasterization size depends on the render scale of its viewport.
            m_fontAtlas = RPI::StreamingImage::CreateFromCpuData(
                *streamingImagePool,
                RHI::ImageDimension::Image2D,
                imageSize,
                RHI::Format::R8G8B8A8_UNORM_SRGB,
                pixels,
                pixelDataSize,
                Uuid::CreateRandom());
            AZ_Error(PassName, m_fontAtlas, "Failed to initialize the ImGui font image!");

            if (m_fontAtlas)
            {
                const uint32_t fontTextureIndex = 0;
                m_resourceGroup->SetImage(m_texturesIndex, m_fontAtlas, fontTextureIndex);
                io.Fonts->TexID = reinterpret_cast<ImTextureID>(m_fontAtlas.get());
                m_imguiFontTexId = io.Fonts->TexID;
            }

            m_fontAtlasFontSize = fontSize;
        }

        uint32_t ImGuiPass::GetRequiredFontSize() const
        {
            if (m_displayWidth == 0 || m_viewportWidth == 0)
            {
                return BaseFontSize;
            }

            // The UI is scaled up to the render target, the glyphs have to be rasterized at that
            // same resolution to end up being pixel accurate. The scale of the display has to be
            // taken into account too, the UI is laid out in device pixels so dpi scaled text ends up
            // being bigger in the render target.
            const float scaleX = static_cast<float>(m_viewportWidth) / static_cast<float>(m_displayWidth);
            const float scaleY = static_cast<float>(m_viewportHeight) / static_cast<float>(m_displayHeight);

            float dpiScale = 1.0f;
            if (const auto* pipeline = GetRenderPipeline())
            {
                if (const auto windowHandle = pipeline->GetWindowHandle())
                {
                    AzFramework::WindowRequestBus::EventResult(
                        dpiScale, windowHandle, &AzFramework::WindowRequestBus::Events::GetDpiScaleFactor);
                }
            }

            const float scale = AZStd::max(scaleX, scaleY) * dpiScale;
            const uint32_t fontSize = aznumeric_cast<uint32_t>(std::lround(static_cast<float>(BaseFontSize) * scale));

            return AZStd::clamp(fontSize, BaseFontSize, BaseFontSize * MaxFontAtlasScale);
        }

        void ImGuiPass::UpdateFontAtlas()
        {
            if (!m_imguiContext || !m_resourceGroup)
            {
                return;
            }

            const uint32_t requiredFontSize = GetRequiredFontSize();
            if (requiredFontSize == m_fontAtlasFontSize)
            {
                m_pendingFontSize = 0;
                return;
            }

            // Wait for the required size to stabilize before rebuilding the atlas, resizing a
            // viewport would otherwise rebuild it on every frame.
            if (m_pendingFontSize != requiredFontSize)
            {
                m_pendingFontSize = requiredFontSize;
                m_pendingFontSizeFrames = 0;
                return;
            }

            if (++m_pendingFontSizeFrames < FontAtlasRebuildDelay)
            {
                return;
            }

            m_pendingFontSize = 0;
            CreateFontAtlas(requiredFontSize);
        }

        void ImGuiPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            Base::SetupFrameGraphDependencies(frameGraph);
            auto imguiContextScope = ImguiContextScope(m_imguiContext);
            ImGui::Render();
            uint32_t drawCount = UpdateImGuiResources();
            frameGraph.SetEstimatedItemCount(drawCount);

            m_drawInfos.NextBuffer();
            m_drawInfos.Get().clear();
            m_drawInfos.Get().reserve(drawCount);
        }

        void ImGuiPass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            // Create all the DrawIndexeds so they can be submitted in parallel on BuildCommandListInternal()
            uint32_t vertexOffset = 0;
            uint32_t indexOffset = 0;
            
            m_userTextures.clear();
            for (ImDrawData& drawData : m_drawData)
            {
                // The clip rects are expressed in the space the draw data was laid out in, convert
                // them to the render target space.
                const AZ::Vector2 scale = GetDrawDataScale(drawData);

                for (int32_t cmdListIdx = 0; cmdListIdx < drawData.CmdListsCount; cmdListIdx++)
                {
                    const ImDrawList* drawList = drawData.CmdLists[cmdListIdx];
                    for (const ImDrawCmd& drawCmd : drawList->CmdBuffer)
                    {
                        AZ_Assert(drawCmd.UserCallback == nullptr, "ImGui UserCallbacks are not supported by the ImGui Pass");
                        int32_t scissorMinX = static_cast<int32_t>(drawCmd.ClipRect.x * scale.GetX());
                        int32_t scissorMinY = static_cast<int32_t>(drawCmd.ClipRect.y * scale.GetY());
                        int32_t scissorMaxX = static_cast<int32_t>(drawCmd.ClipRect.z * scale.GetX());
                        int32_t scissorMaxY = static_cast<int32_t>(drawCmd.ClipRect.w * scale.GetY());

                        //scissorMaxX/scissorMaxY can be a frame stale from imgui (ImGui::NewFrame runs after this) hence we clamp it to viewport bounds
                        //otherwise it is possible to have a frame where scissor bounds can be bigger than window's bounds if we resize the window
                        scissorMaxX = AZStd::min(scissorMaxX, static_cast<int32_t>(m_viewportWidth));
                        scissorMaxY = AZStd::min(scissorMaxY, static_cast<int32_t>(m_viewportHeight));

                        // check if texture id needs to be bound to the srg
                        uint32_t index = 0;
                        if (drawCmd.TextureId && drawCmd.TextureId != m_imguiFontTexId)
                        {
                            if (m_userTextures.size() < MaxUserTextures)
                            {
                                Data::Instance<RPI::StreamingImage> img = reinterpret_cast<RPI::StreamingImage*>(drawCmd.TextureId);
                                // Texture index 0 is reserved for the font atlas, so we start from 1 to 15 for user textures.
                                index = aznumeric_cast<uint32_t>(m_userTextures.size() + 1);
                                m_userTextures[img.get()] = index;
                            }
                            else
                            {
                                AZ_Warning("ImGuiPass", false, "The maximum number of textures ImGui can render per frame is %d", MaxUserTextures);
                            }
                        }

                        RHI::GeometryView geometryView{ RHI::MultiDevice::AllDevices };
                        geometryView.SetDrawArguments(RHI::DrawIndexed(vertexOffset, drawCmd.ElemCount, indexOffset));
                        geometryView.SetIndexBufferView(m_indexBufferView);
                        geometryView.AddStreamBufferView(m_vertexBufferView[0]);
                        geometryView.AddStreamBufferView(m_vertexBufferView[1]);

                        m_drawInfos.Get().push_back(
                            {
                                RHI::DrawInstanceArguments(1, index),
                                geometryView,
                                RHI::Scissor(
                                    scissorMinX,
                                    scissorMinY,
                                    scissorMaxX,
                                    scissorMaxY
                                )
                            }
                        );

                        indexOffset += drawCmd.ElemCount;
                    }
                    vertexOffset += drawList->VtxBuffer.size();
                }
            }
            m_drawData.clear();
            
            auto imguiContextScope = ImguiContextScope(m_imguiContext);
            ImGui::GetIO().MouseWheel = m_lastFrameMouseWheel;
            m_lastFrameMouseWheel = 0.0;

            for (auto& [streamingImage, index] : m_userTextures)
            {
                m_resourceGroup->SetImage(m_texturesIndex, streamingImage, index);
            }
            m_resourceGroup->Compile();
        }

        void ImGuiPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            AZ_PROFILE_SCOPE(AzRender, "ImGuiPass: BuildCommandListInternal");

            context.GetCommandList()->SetViewport(m_viewportState);
            context.GetCommandList()->SetShaderResourceGroupForDraw(*m_resourceGroup->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(context.GetDeviceIndex()));

            for (uint32_t i = context.GetSubmitRange().m_startIndex; i < context.GetSubmitRange().m_endIndex; ++i)
            {
                DrawInfo& drawInfo = m_drawInfos.Get().at(i);
                RHI::DeviceDrawItem drawItem;
                drawItem.m_drawInstanceArgs = drawInfo.m_drawInstanceArgs;
                drawItem.m_geometryView = drawInfo.m_geometryView.GetDeviceGeometryView(context.GetDeviceIndex());
                drawItem.m_streamIndices = drawInfo.m_geometryView.GetFullStreamBufferIndices();
                drawItem.m_pipelineState = m_pipelineState->GetRHIPipelineState()->GetDevicePipelineState(context.GetDeviceIndex()).get();
                drawItem.m_scissorsCount = 1;
                drawItem.m_scissors = &drawInfo.m_scissor;

                context.GetCommandList()->Submit(drawItem, i);
            }
        }

        uint32_t ImGuiPass::UpdateImGuiResources()
        {
            AZ_PROFILE_SCOPE(AzRender, "ImGuiPass: UpdateImGuiResources");

            auto imguiContextScope = ImguiContextScope(m_imguiContext);

            constexpr uint32_t indexSize = aznumeric_cast<uint32_t>(sizeof(ImDrawIdx));
            constexpr uint32_t vertexSize = aznumeric_cast<uint32_t>(sizeof(ImDrawVert));

            if (ImGui::GetDrawData())
            {
                m_drawData.push_back(*ImGui::GetDrawData());
            }

            uint32_t totalIdxBufferSize = 0;
            uint32_t totalVtxBufferSize = 0;
            for (ImDrawData& drawData : m_drawData)
            {
                totalIdxBufferSize += drawData.TotalIdxCount * indexSize;
                totalVtxBufferSize += drawData.TotalVtxCount * vertexSize;
            }

            if (totalIdxBufferSize == 0)
            {
                return 0; // Nothing to draw.
            }

            auto vertexBuffer = RPI::DynamicDrawInterface::Get()->GetDynamicBuffer(totalVtxBufferSize, RHI::Alignment::InputAssembly);
            auto indexBuffer = RPI::DynamicDrawInterface::Get()->GetDynamicBuffer(totalIdxBufferSize, RHI::Alignment::InputAssembly);

            if (!vertexBuffer || !indexBuffer)
            {
                return 0;
            }

            auto indexBufferDataAddress = indexBuffer->GetBufferAddress();
            auto vertexBufferDataAddress = vertexBuffer->GetBufferAddress();

            uint32_t drawCount = 0;
            uint32_t indexBufferOffset = 0;
            uint32_t vertexBufferOffset = 0;

            for (ImDrawData& drawData : m_drawData)
            {
                // The positions are expressed in the space the draw data was laid out in, convert
                // them to the render target space.
                const AZ::Vector2 scale = GetDrawDataScale(drawData);

                for (int32_t cmdListIndex = 0; cmdListIndex < drawData.CmdListsCount; cmdListIndex++)
                {
                    const ImDrawList* drawList = drawData.CmdLists[cmdListIndex];

                    const uint32_t indexBufferByteSize = drawList->IdxBuffer.size() * indexSize;
                    for(auto [deviceIndex, indexBufferData] : indexBufferDataAddress)
                    {
                        memcpy(static_cast<ImDrawIdx*>(indexBufferData) + indexBufferOffset, drawList->IdxBuffer.Data, indexBufferByteSize);
                    }
                    indexBufferOffset += drawList->IdxBuffer.size();

                    const uint32_t vertexBufferByteSize = drawList->VtxBuffer.size() * vertexSize;
                    for(auto [deviceIndex, vertexBufferData] : vertexBufferDataAddress)
                    {
                        ImDrawVert* vertices = static_cast<ImDrawVert*>(vertexBufferData) + vertexBufferOffset;
                        memcpy(vertices, drawList->VtxBuffer.Data, vertexBufferByteSize);

                        if (scale != AZ::Vector2::CreateOne())
                        {
                            for (uint32_t vertexIndex = 0; vertexIndex < drawList->VtxBuffer.size(); ++vertexIndex)
                            {
                                vertices[vertexIndex].pos.x *= scale.GetX();
                                vertices[vertexIndex].pos.y *= scale.GetY();
                            }
                        }
                    }
                    vertexBufferOffset += drawList->VtxBuffer.size();

                    drawCount += drawList->CmdBuffer.size();
                }
            }

            static_assert(indexSize == 2, "Expected index size from ImGui to be 2 to match RHI::IndexFormat::Uint16");
            m_indexBufferView = indexBuffer->GetIndexBufferView(RHI::IndexFormat::Uint16);
            m_vertexBufferView[0] = vertexBuffer->GetStreamBufferView(vertexSize);
            m_vertexBufferView[1] = m_instanceBufferView;

            RHI::ValidateStreamBufferViews(m_pipelineState->ConstDescriptor().m_inputStreamLayout, m_vertexBufferView);

            return drawCount;
        }

    } // namespace Render
} // namespace AZ
