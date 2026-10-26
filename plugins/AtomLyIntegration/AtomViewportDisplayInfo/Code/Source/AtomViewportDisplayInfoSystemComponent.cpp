/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomViewportDisplayInfoSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <Atom/Feature/ImGui/ImGuiPass.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIMemoryStatisticsInterface.h>
#include <Atom/RHI.Reflect/MemoryUsage.h>

#include <imgui/imgui.h>

namespace AZ::Render
{
    AZ_CVAR(int, r_displayInfo, 2, [](const int& newDisplayInfoVal)->void
        {
            // Forward this event to the system component so it can update accordingly.
            // This callback only gets triggered by console commands, so this will not recurse.
            AtomBridge::AtomViewportInfoDisplayRequestBus::Broadcast(
                &AtomBridge::AtomViewportInfoDisplayRequestBus::Events::SetDisplayState,
                aznumeric_cast<AtomBridge::ViewportInfoDisplayState>(newDisplayInfoVal)
            );
        }, AZ::ConsoleFunctorFlags::DontReplicate,
        "Toggles debugging information display.\n"
        "Usage: r_displayInfo [0=off/1=show/2=enhanced/3=compact]"
    );
    AZ_CVAR(float, r_fpsCalcInterval, 1.0f, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "The time period over which to calculate the framerate for r_displayInfo."
    );

    void AtomViewportDisplayInfoSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomViewportDisplayInfoSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AtomViewportDisplayInfoSystemComponent>("Viewport Display Info", "Manages debug viewport information through r_displayInfo")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AtomViewportDisplayInfoSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ViewportDisplayInfoService"));
    }

    void AtomViewportDisplayInfoSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ViewportDisplayInfoService"));
    }

    void AtomViewportDisplayInfoSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void AtomViewportDisplayInfoSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void AtomViewportDisplayInfoSystemComponent::Activate()
    {
        AZ::Name apiName = AZ::RHI::Factory::Get().GetName();
        if (!apiName.IsEmpty())
        {
            m_rendererDescription = AZStd::string::format("%s", apiName.GetCStr());
        }

        AZ::TickBus::Handler::BusConnect();
        AZ::Render::ImGuiSystemNotificationBus::Handler::BusConnect();
        AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Handler::BusConnect();
    }

    void AtomViewportDisplayInfoSystemComponent::Deactivate()
    {
        AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Handler::BusDisconnect();
        AZ::Render::ImGuiSystemNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void AtomViewportDisplayInfoSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        UpdateFramerate();
    }

    void AtomViewportDisplayInfoSystemComponent::ActiveImGuiContextChanged([[maybe_unused]] ImGuiContext* context)
    {
    }

    void AtomViewportDisplayInfoSystemComponent::OnImGuiPassFrame(ImGuiPass* imguiPass)
    {
        const AtomBridge::ViewportInfoDisplayState displayLevel = GetDisplayState();
        if (displayLevel == AtomBridge::ViewportInfoDisplayState::NoInfo || imguiPass == nullptr)
        {
            return;
        }

        RPI::RenderPipeline* pipeline = imguiPass->GetRenderPipeline();
        if (pipeline == nullptr)
        {
            return;
        }

        // The overlay is laid out in the on-screen space of the viewport (the client area of its
        // window), so it keeps the same size on screen whatever the internal render resolution is.
        AzFramework::WindowSize renderSize;
        float dpiScalingFactor = 1.0f;
        if (const AzFramework::NativeWindowHandle windowHandle = pipeline->GetWindowHandle())
        {
            AzFramework::WindowRequestBus::EventResult(
                renderSize, windowHandle, &AzFramework::WindowRequestBus::Events::GetRenderResolution);
            AzFramework::WindowRequestBus::EventResult(
                dpiScalingFactor, windowHandle, &AzFramework::WindowRequestBus::Events::GetDpiScaleFactor);
        }

        if (m_updateRootPassQuery)
        {
            if (auto rootPass = pipeline->GetRootPass())
            {
                rootPass->SetPipelineStatisticsQueryEnabled(displayLevel != AtomBridge::ViewportInfoDisplayState::CompactInfo);
                m_updateRootPassQuery = false;
            }
        }

        // The overlay is drawn directly with the ImGui primitives of the pass, without any window, so
        // it looks like a plain HUD on top of the image instead of a movable window.
        m_drawList = ImGui::GetBackgroundDrawList();
        m_font = ImGui::GetFont();

        // The size of the UI in the layout space of the pass. It is computed from the font instead of
        // ImGui::GetFontSize(), which is only valid while an ImGui window is being built.
        m_fontSize = ImGui::GetIO().FontGlobalScale * m_font->FontSize * m_font->Scale * dpiScalingFactor;
        m_lineSpacing = m_fontSize * 0.5f;
        m_lineAdvance = m_fontSize + m_lineSpacing;
        m_bgPad = 4.0f * dpiScalingFactor;
        m_bgX = 10.0f * dpiScalingFactor - m_bgPad;
        m_bgW = 260.0f * dpiScalingFactor;
        m_positionY = 22.0f * dpiScalingFactor;

        AZStd::vector<Line> lines;

        DrawFramerate(lines);
        DrawSection(lines, AZ::Color(0.0f, 0.15f, 0.2f, 0.9f));

        lines.clear();
        DrawMemoryInfo(lines);
        DrawSection(lines, AZ::Color(0.0f, 0.05f, 0.18f, 0.85f));

        if (displayLevel != AtomBridge::ViewportInfoDisplayState::CompactInfo)
        {
            lines.clear();
            DrawPassInfo(lines);
            DrawSection(lines, AZ::Color(0.1f, 0.0f, 0.18f, 0.85f));
        }

        lines.clear();
        DrawRendererInfo(lines, renderSize, pipeline);
        DrawSection(lines, AZ::Color(0.15f, 0.07f, 0.0f, 0.85f));

        if (displayLevel == AtomBridge::ViewportInfoDisplayState::FullInfo)
        {
            lines.clear();
            DrawCameraInfo(lines, pipeline);
            DrawSection(lines, AZ::Color(0.0f, 0.14f, 0.07f, 0.85f));
        }

        m_drawList = nullptr;
    }

    AtomBridge::ViewportInfoDisplayState AtomViewportDisplayInfoSystemComponent::GetDisplayState() const
    {
        return aznumeric_cast<AtomBridge::ViewportInfoDisplayState>(r_displayInfo.operator int());
    }

    void AtomViewportDisplayInfoSystemComponent::SetDisplayState(AtomBridge::ViewportInfoDisplayState state)
    {
        r_displayInfo = aznumeric_cast<int>(state);
        m_updateRootPassQuery = true;
        AtomBridge::AtomViewportInfoDisplayNotificationBus::Broadcast(
            &AtomBridge::AtomViewportInfoDisplayNotificationBus::Events::OnViewportInfoDisplayStateChanged,
            state);
    }

    void AtomViewportDisplayInfoSystemComponent::AddLine(AZStd::vector<Line>& lines, AZStd::string text, const AZ::Color& color)
    {
        lines.push_back(Line{ AZStd::move(text), color });
    }

    void AtomViewportDisplayInfoSystemComponent::DrawSection(const AZStd::vector<Line>& lines, const AZ::Color& backgroundColor)
    {
        if (lines.empty())
        {
            return;
        }

        const ImU32 background = ImGui::ColorConvertFloat4ToU32(
            ImVec4(backgroundColor.GetR(), backgroundColor.GetG(), backgroundColor.GetB(), backgroundColor.GetA()));

        // The background of the section is drawn behind the text of the section.
        const float sectionHeight = lines.size() * m_lineAdvance - m_lineSpacing + m_bgPad * 2.0f;
        m_drawList->AddRectFilled(
            ImVec2(m_bgX, m_positionY - m_bgPad),
            ImVec2(m_bgX + m_bgW, m_positionY - m_bgPad + sectionHeight),
            background);

        for (const Line& line : lines)
        {
            const ImU32 color = ImGui::ColorConvertFloat4ToU32(
                ImVec4(line.m_color.GetR(), line.m_color.GetG(), line.m_color.GetB(), line.m_color.GetA()));

            const char* textBegin = line.m_text.c_str();
            m_drawList->AddText(
                m_font, m_fontSize, ImVec2(m_bgX + m_bgPad, m_positionY), color, textBegin, textBegin + line.m_text.size());
            m_positionY += m_lineAdvance;
        }
    }

    void AtomViewportDisplayInfoSystemComponent::UpdateFramerate()
    {
        auto currentTime = AZStd::chrono::steady_clock::now();
        while (!m_fpsHistory.empty() && (currentTime - m_fpsHistory.front()) > m_fpsInterval)
        {
            m_fpsHistory.pop_front();
        }
        m_fpsHistory.push_back(currentTime);
    }

    void AtomViewportDisplayInfoSystemComponent::DrawFramerate(AZStd::vector<Line>& lines) const
    {
        AZStd::optional<AZStd::chrono::steady_clock::time_point> lastTime;
        double minFPS = DBL_MAX;
        double maxFPS = 0;
        AZStd::chrono::duration<double> deltaTime;
        for (const auto& time : m_fpsHistory)
        {
            if (lastTime.has_value())
            {
                deltaTime = time - lastTime.value();
                double fps = AZStd::chrono::seconds(1) / deltaTime;
                minFPS = AZStd::min(minFPS, fps);
                maxFPS = AZStd::max(maxFPS, fps);
            }
            lastTime = time;
        }

        double averageFPS = 0;
        double averageFrameMs = 0;
        if (m_fpsHistory.size() > 1)
        {
            deltaTime = m_fpsHistory.back() - m_fpsHistory.front();
            averageFPS = AZStd::chrono::seconds(m_fpsHistory.size()) / deltaTime;
            averageFrameMs = 1000.0f / averageFPS;
        }

        auto ClampedFloatDisplay = [](double value, const char* format) -> AZStd::string
        {
            constexpr float upperLimit = 10000.0f;
            return value > upperLimit ? "inf" : AZStd::string::format(format, value);
        };

        AddLine(lines,
            AZStd::string::format("FPS  %s", ClampedFloatDisplay(averageFPS, "%.1f").c_str()),
            AZ::Colors::White);
        AddLine(lines,
            AZStd::string::format(
                "Frame  %s ms  [%s .. %s]",
                ClampedFloatDisplay(averageFrameMs, "%.1f").c_str(),
                ClampedFloatDisplay(minFPS, "%.0f").c_str(),
                ClampedFloatDisplay(maxFPS, "%.0f").c_str()),
            AZ::Colors::White);
    }

    void AtomViewportDisplayInfoSystemComponent::DrawMemoryInfo(AZStd::vector<Line>& lines) const
    {
        RHI::RHISystemInterface* rhi = RHI::RHISystemInterface::Get();
        if (!rhi)
        {
            return;
        }

        RHI::RHIMemoryStatisticsInterface* rhiMemStats = RHI::RHIMemoryStatisticsInterface::Get();
        if (!rhiMemStats)
        {
            return;
        }

        const RHI::MemoryStatistics* stats = rhiMemStats->GetMemoryStatistics();
        if (!stats)
        {
            return;
        }

        // Accumulate total device memory pressure (reserved, resident)
        size_t deviceResident = 0;
        size_t deviceReserved = 0;

        for (const auto& heap : stats->m_heaps)
        {
            if (heap.m_heapMemoryType == RHI::HeapMemoryLevel::Device)
            {
                deviceResident += heap.m_memoryUsage.m_usedResidentInBytes;
                deviceReserved += heap.m_memoryUsage.m_totalResidentInBytes;
            }
        }

        // Fallback if the information from the heaps is not available, we use the pools instead.
        if (deviceResident == 0 || deviceReserved == 0)
        {
            deviceResident = deviceReserved = 0;
            for (const auto& pool : stats->m_pools)
            {
                deviceReserved += pool.m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device).m_totalResidentInBytes;
                deviceResident += pool.m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device).m_usedResidentInBytes;
            }
        }

        // Query for available device memory
        float availableDeviceMemoryMB = 0.f;

        static constexpr size_t MB = 1u << 20;

        if (RHI::Device* device = rhi->GetDevice(); device)
        {
            const RHI::PhysicalDeviceDescriptor& deviceDesc = device->GetPhysicalDevice().GetDescriptor();
            availableDeviceMemoryMB =
                static_cast<float>(deviceDesc.m_heapSizePerLevel[static_cast<size_t>(RHI::HeapMemoryLevel::Device)]) / MB;
        }

        float deviceResidentMB = static_cast<float>(deviceResident) / MB;

        AZ::Color deviceMemoryColor = AZ::Color(0.4f, 0.85f, 1.0f, 1.0f);
        if (availableDeviceMemoryMB != 0.f)
        {
            // Highlight text based on device memory pressure
            if (deviceResidentMB > 0.6f * availableDeviceMemoryMB)
            {
                deviceMemoryColor = AZ::Colors::Yellow;
            }
            else if (deviceResidentMB > 0.8f * availableDeviceMemoryMB)
            {
                deviceMemoryColor = AZ::Colors::Red;
            }
        }
        AddLine(lines,
            AZStd::string::format("VRAM  %.0f / %.0f MiB", deviceResidentMB, availableDeviceMemoryMB),
            deviceMemoryColor);

        // RPI default StreamingImagePool usage
        Data::Instance<RPI::StreamingImagePool> streamingImagePool = RPI::ImageSystemInterface::Get()->GetSystemStreamingPool();
        const RHI::HeapMemoryUsage& imagePoolMemoryUsage = streamingImagePool->GetRHIPool()->GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);

        float imagePoolUsedAllocatedMB = static_cast<float>(imagePoolMemoryUsage.m_usedResidentInBytes) / MB;
        float imagePoolBudgetMB = static_cast<float>(imagePoolMemoryUsage.m_budgetInBytes) / MB;
        AZ::Color fontColor = AZ::Color(0.4f, 0.85f, 1.0f, 1.0f);
        if (streamingImagePool->IsMemoryLow())
        {
            fontColor = AZ::Colors::Red;
        }

        AddLine(lines,
            AZStd::string::format("Textures  %.0f / %.0f MiB", imagePoolUsedAllocatedMB, imagePoolBudgetMB),
            fontColor);
    }

    void AtomViewportDisplayInfoSystemComponent::DrawPassInfo(AZStd::vector<Line>& lines) const
    {
        RPI::PassSystemFrameStatistics passSystemFrameStatistics = AZ::RPI::PassSystemInterface::Get()->GetFrameStatistics();

        AddLine(lines,
            AZStd::string::format("Passes  %d", passSystemFrameStatistics.m_numRenderPassesExecuted),
            AZ::Color(0.75f, 0.5f, 1.0f, 1.0f));
        AddLine(lines,
            AZStd::string::format(
                "Draw Calls  %d  |  Max  %d",
                passSystemFrameStatistics.m_totalDrawItemsRendered,
                passSystemFrameStatistics.m_maxDrawItemsRenderedInAPass),
            AZ::Color(0.75f, 0.5f, 1.0f, 1.0f));
    }

    void AtomViewportDisplayInfoSystemComponent::DrawRendererInfo(
        AZStd::vector<Line>& lines, const AzFramework::WindowSize& renderSize, RPI::RenderPipeline* pipeline) const
    {
        const AZ::Color rendColor(1.0f, 0.8f, 0.3f, 1.0f);
        AddLine(lines, m_rendererDescription, rendColor);

        // resolution and MSAA state
        const RHI::MultisampleState& multisampleState = RPI::RPISystemInterface::Get()->GetApplicationMultisampleState();

        AZStd::string defaultAA = "MSAA";
        bool hasAAMethod = false;
        AZ::RPI::AntiAliasingMode defaultAAMethod = pipeline->GetActiveAAMethod();
        defaultAA = AZ::RPI::RenderPipeline::GetAAMethodNameByIndex(defaultAAMethod);
        hasAAMethod = (defaultAAMethod != AZ::RPI::AntiAliasingMode::MSAA && defaultAAMethod != AZ::RPI::AntiAliasingMode::Default);

        auto resolutionStr = AZStd::string::format("%dx%d", renderSize.m_width, renderSize.m_height);
        auto msaaStr =
            multisampleState.m_samples > 1 ? AZStd::string::format("MSAA %dx", multisampleState.m_samples) : AZStd::string("CMAA2");

        if (hasAAMethod)
        {
            if (multisampleState.m_samples > 1)
            {
                AddLine(lines, AZStd::string::format("%s  %s + %s", resolutionStr.c_str(), defaultAA.c_str(), msaaStr.c_str()), rendColor);
            }
            else
            {
                AddLine(lines, AZStd::string::format("%s  %s", resolutionStr.c_str(), defaultAA.c_str()), rendColor);
            }
        }
        else
        {
            AddLine(lines, AZStd::string::format("%s  %s", resolutionStr.c_str(), msaaStr.c_str()), rendColor);
        }

        AddLine(lines, AZStd::string::format("Pipeline  %s", pipeline->GetId().GetCStr()), rendColor);
    }

    void AtomViewportDisplayInfoSystemComponent::DrawCameraInfo(AZStd::vector<Line>& lines, RPI::RenderPipeline* pipeline) const
    {
        AZ::RPI::ViewPtr currentView = pipeline->GetDefaultView();
        if (currentView == nullptr)
        {
            return;
        }

        AzFramework::CameraState cameraState;
        AzFramework::SetCameraClippingVolumeFromPerspectiveFovMatrixRH(cameraState, currentView->GetViewToClipMatrix());
        const AZ::Transform transform = currentView->GetCameraTransform();
        const AZ::Vector3 translation = transform.GetTranslation();
        const AZ::Vector3 rotation = transform.GetEulerDegrees();
        const AZ::Color camColor(0.4f, 1.0f, 0.65f, 1.0f);

        AddLine(lines,
            AZStd::string::format(
                "CamPos=%.2f %.2f %.2f",
                translation.GetX(), translation.GetY(), translation.GetZ()),
            camColor);
        AddLine(lines,
            AZStd::string::format(
                "Angl=%3.0f %3.0f %4.0f ZN=%.2f ZF=%.0f",
                rotation.GetX(), rotation.GetY(), rotation.GetZ(),
                cameraState.m_nearClip, cameraState.m_farClip),
            camColor);
    }
} // namespace AZ::Render
