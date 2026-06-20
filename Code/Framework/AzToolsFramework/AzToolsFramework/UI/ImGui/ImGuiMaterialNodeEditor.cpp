#include "EditorDefs.h"
#include "ImGuiMaterialNodeEditor.h"

#include <imgui/imgui.h>
#include <imgui_node_editor.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace ed = ax::NodeEditor;

namespace ImGuiMaterialNodeEditor
{
    static const ImVec4 O3DE_ACCENT_GREEN = ImVec4(0.29f, 0.48f, 0.30f, 1.0f);

    enum class PinType
    {
        Float,
        Float2,
        Float3,
        Float4,
        Color,
        Texture,
        Bool
    };

    enum class NodeType
    {
        ConstantFloat,
        ConstantColor,
        TextureSample,
        TextureCoordinate,
        MathAdd,
        MathMultiply,
        MathLerp,
        Output
    };

    static ImColor GetPinColor(PinType type)
    {
        switch (type)
        {
        case PinType::Float:    return ImColor(140, 140, 140);
        case PinType::Float2:   return ImColor(140, 200, 140);
        case PinType::Float3:   return ImColor(140, 140, 240);
        case PinType::Float4:   return ImColor(200, 140, 240);
        case PinType::Color:    return ImColor(240, 200, 100);
        case PinType::Texture:  return ImColor(200, 140, 100);
        case PinType::Bool:     return ImColor(240, 100, 100);
        default:                return ImColor(255, 255, 255);
        }
    }

    static const char* GetPinTypeName(PinType type)
    {
        switch (type)
        {
        case PinType::Float:    return "Float";
        case PinType::Float2:   return "Float2";
        case PinType::Float3:   return "Float3";
        case PinType::Float4:   return "Float4";
        case PinType::Color:    return "Color";
        case PinType::Texture:  return "Texture";
        case PinType::Bool:     return "Bool";
        default:                return "";
        }
    }

    struct Pin
    {
        ed::PinId Id;
        AZStd::string Name;
        PinType Type;
        bool IsInput;
        int NodeId;
    };

    struct Link
    {
        ed::LinkId Id;
        ed::PinId StartPinId;
        ed::PinId EndPinId;
    };

    struct MaterialNode
    {
        ed::NodeId Id;
        NodeType Type;
        AZStd::string Name;
        ImVec2 Position;
        AZStd::vector<Pin> Inputs;
        AZStd::vector<Pin> Outputs;
        float FloatValues[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        AZStd::string TexturePath;
    };

    static ed::EditorContext* s_editorContext = nullptr;
    static AZStd::vector<MaterialNode> s_nodes;
    static AZStd::vector<Link> s_links;
    static int s_nextNodeId = 1;
    static int s_nextPinId = 1;
    static int s_nextLinkId = 1;
    static AZStd::string s_currentMaterialPath;
    static ed::NodeId s_selectedNodeId;
    static bool s_hasSelection = false;

    static int GetNextNodeId() { return s_nextNodeId++; }
    static int GetNextPinId() { return s_nextPinId++; }
    static int GetNextLinkId() { return s_nextLinkId++; }

    static Pin& FindPin(ed::PinId id)
    {
        for (auto& node : s_nodes)
        {
            for (auto& pin : node.Inputs)
            {
                if (pin.Id == id)
                    return pin;
            }
            for (auto& pin : node.Outputs)
            {
                if (pin.Id == id)
                    return pin;
            }
        }
        static Pin dummy;
        return dummy;
    }

    static MaterialNode& FindNode(ed::NodeId id)
    {
        for (auto& node : s_nodes)
        {
            if (node.Id == id)
                return node;
        }
        static MaterialNode dummy;
        return dummy;
    }

    static void AddPin(MaterialNode& node, const char* name, PinType type, bool isInput)
    {
        Pin pin;
        pin.Id = ed::PinId(GetNextPinId());
        pin.Name = name;
        pin.Type = type;
        pin.IsInput = isInput;
        pin.NodeId = node.Id.Get();
        if (isInput)
            node.Inputs.push_back(pin);
        else
            node.Outputs.push_back(pin);
    }

    static MaterialNode& CreateNode(NodeType type, const char* name, ImVec2 pos)
    {
        MaterialNode node;
        node.Id = ed::NodeId(GetNextNodeId());
        node.Type = type;
        node.Name = name;
        node.Position = pos;

        switch (type)
        {
        case NodeType::ConstantFloat:
            AddPin(node, "Value", PinType::Float, false);
            node.FloatValues[0] = 0.0f;
            break;
        case NodeType::ConstantColor:
            AddPin(node, "Color", PinType::Color, false);
            node.FloatValues[0] = 1.0f;
            node.FloatValues[1] = 1.0f;
            node.FloatValues[2] = 1.0f;
            node.FloatValues[3] = 1.0f;
            break;
        case NodeType::TextureSample:
            AddPin(node, "UV", PinType::Float2, true);
            AddPin(node, "RGBA", PinType::Float4, false);
            AddPin(node, "R", PinType::Float, false);
            AddPin(node, "G", PinType::Float, false);
            AddPin(node, "B", PinType::Float, false);
            AddPin(node, "A", PinType::Float, false);
            break;
        case NodeType::TextureCoordinate:
            AddPin(node, "UV", PinType::Float2, false);
            break;
        case NodeType::MathAdd:
            AddPin(node, "A", PinType::Float, true);
            AddPin(node, "B", PinType::Float, true);
            AddPin(node, "Result", PinType::Float, false);
            break;
        case NodeType::MathMultiply:
            AddPin(node, "A", PinType::Float, true);
            AddPin(node, "B", PinType::Float, true);
            AddPin(node, "Result", PinType::Float, false);
            break;
        case NodeType::MathLerp:
            AddPin(node, "A", PinType::Float, true);
            AddPin(node, "B", PinType::Float, true);
            AddPin(node, "Alpha", PinType::Float, true);
            AddPin(node, "Result", PinType::Float, false);
            break;
        case NodeType::Output:
            AddPin(node, "BaseColor", PinType::Color, true);
            AddPin(node, "Emissive", PinType::Color, true);
            AddPin(node, "Roughness", PinType::Float, true);
            AddPin(node, "Metallic", PinType::Float, true);
            AddPin(node, "Normal", PinType::Float3, true);
            AddPin(node, "Opacity", PinType::Float, true);
            break;
        }

        s_nodes.push_back(node);
        return s_nodes.back();
    }

    static bool CanCreateLink(Pin& startPin, Pin& endPin)
    {
        if (startPin.Type != endPin.Type)
            return false;
        if (startPin.IsInput == endPin.IsInput)
            return false;
        if (startPin.NodeId == endPin.NodeId)
            return false;
        return true;
    }

    static void RenderPinIcon(PinType type, bool connected, float alpha)
    {
        ImColor color = GetPinColor(type);
        color.Value.w = alpha;
        float radius = 6.0f;
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 center = ImGui::GetCursorScreenPos();
        center.x += radius;
        center.y += radius;
        if (connected)
            drawList->AddCircleFilled(center, radius, color);
        else
            drawList->AddCircle(center, radius, color, 0, 2.0f);
    }

    static void RenderNode(MaterialNode& node)
    {
        ed::BeginNode(node.Id);
        ImGui::PushID(node.Id.Get());
        ImGui::TextColored(O3DE_ACCENT_GREEN, "%s", node.Name.c_str());
        ImGui::Separator();

        for (auto& input : node.Inputs)
        {
            ed::BeginPin(input.Id, ed::PinKind::Input);
            RenderPinIcon(input.Type, false, 1.0f);
            ImGui::SameLine();
            ImGui::Text("%s", input.Name.c_str());
            ed::EndPin();
        }

        switch (node.Type)
        {
        case NodeType::ConstantFloat:
            ImGui::SetNextItemWidth(100.0f);
            ImGui::DragFloat("##val", &node.FloatValues[0], 0.01f, -100.0f, 100.0f);
            break;
        case NodeType::ConstantColor:
        {
            float col[4] = { node.FloatValues[0], node.FloatValues[1], node.FloatValues[2], node.FloatValues[3] };
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::ColorEdit4("##col", col, ImGuiColorEditFlags_NoInputs))
            {
                node.FloatValues[0] = col[0];
                node.FloatValues[1] = col[1];
                node.FloatValues[2] = col[2];
                node.FloatValues[3] = col[3];
            }
            break;
        }
        case NodeType::TextureSample:
        {
            char buf[256] = {};
            strncpy(buf, node.TexturePath.c_str(), sizeof(buf) - 1);
            ImGui::SetNextItemWidth(150.0f);
            if (ImGui::InputText("##tex", buf, sizeof(buf)))
                node.TexturePath = buf;
            break;
        }
        default:
            break;
        }

        for (auto& output : node.Outputs)
        {
            ed::BeginPin(output.Id, ed::PinKind::Output);
            ImGui::Text("%s", output.Name.c_str());
            ImGui::SameLine();
            RenderPinIcon(output.Type, false, 1.0f);
            ed::EndPin();
        }

        ImGui::PopID();
        ed::EndNode();
    }

    static void RenderRightPanel()
    {
        ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoCollapse);

        ed::NodeId selected = s_selectedNodeId;
        if (!selected)
        {
            ImGui::TextDisabled("Select a node to edit");
            ImGui::End();
            return;
        }

        MaterialNode& node = FindNode(selected);
        if (node.Id.Get() == 0)
        {
            ImGui::End();
            return;
        }

        ImGui::TextColored(O3DE_ACCENT_GREEN, "%s", node.Name.c_str());
        ImGui::Separator();
        ImGui::Text("Inputs:");
        for (auto& pin : node.Inputs)
            ImGui::BulletText("%s [%s]", pin.Name.c_str(), GetPinTypeName(pin.Type));
        ImGui::Text("Outputs:");
        for (auto& pin : node.Outputs)
            ImGui::BulletText("%s [%s]", pin.Name.c_str(), GetPinTypeName(pin.Type));

        if (node.Type == NodeType::ConstantFloat)
        {
            ImGui::Separator();
            ImGui::SetNextItemWidth(150.0f);
            ImGui::DragFloat("Value", &node.FloatValues[0], 0.01f);
        }
        else if (node.Type == NodeType::ConstantColor)
        {
            ImGui::Separator();
            float col[4] = { node.FloatValues[0], node.FloatValues[1], node.FloatValues[2], node.FloatValues[3] };
            ImGui::ColorEdit4("Color", col);
            node.FloatValues[0] = col[0];
            node.FloatValues[1] = col[1];
            node.FloatValues[2] = col[2];
            node.FloatValues[3] = col[3];
        }

        if (ImGui::Button("Delete Node", ImVec2(180, 24)))
        {
            s_links.erase(
                std::remove_if(s_links.begin(), s_links.end(),
                    [&](const Link& link) {
                        return FindPin(link.StartPinId).NodeId == node.Id.Get()
                            || FindPin(link.EndPinId).NodeId == node.Id.Get();
                    }),
                s_links.end()
            );
            s_nodes.erase(
                std::remove_if(s_nodes.begin(), s_nodes.end(),
                    [&](const MaterialNode& n) { return n.Id == node.Id; }),
                s_nodes.end()
            );
            s_selectedNodeId = ed::NodeId(0);
            s_hasSelection = false;
        }

        ImGui::End();
    }

    void Initialize()
    {
        s_nodes.clear();
        s_links.clear();
        s_nextNodeId = 1;
        s_nextPinId = 1;
        s_nextLinkId = 1;
        s_currentMaterialPath.clear();

        ed::Config config;
        config.SettingsFile = nullptr;
        s_editorContext = ed::CreateEditor(&config);
    }

    void Shutdown()
    {
        if (s_editorContext)
        {
            ed::DestroyEditor(s_editorContext);
            s_editorContext = nullptr;
        }
        s_nodes.clear();
        s_links.clear();
    }

    void LoadMaterial(const char* path)
    {
        s_nodes.clear();
        s_links.clear();
        s_nextNodeId = 1;
        s_nextPinId = 1;
        s_nextLinkId = 1;
        s_currentMaterialPath = path;

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO || !fileIO->Exists(path))
        {
            CreateNode(NodeType::Output, "Output", ImVec2(400, 200));
            auto& baseColor = CreateNode(NodeType::ConstantColor, "Base Color", ImVec2(100, 100));
            baseColor.FloatValues[0] = 0.5f;
            baseColor.FloatValues[1] = 0.5f;
            baseColor.FloatValues[2] = 0.5f;
            baseColor.FloatValues[3] = 1.0f;
            return;
        }

        AZ::IO::FileIOStream file(path, AZ::IO::OpenMode::ModeRead);
        if (!file.IsOpen())
            return;

        AZ::u64 size = file.GetLength();
        AZStd::string content;
        content.resize(static_cast<size_t>(size));
        file.Read(content.size(), content.data());
        file.Close();

        rapidjson::Document doc;
        doc.Parse(content.c_str());
        if (doc.HasParseError())
        {
            CreateNode(NodeType::Output, "Output", ImVec2(400, 200));
            return;
        }

        auto& output = CreateNode(NodeType::Output, "Output", ImVec2(400, 200));
        ImVec2 pos(100, 100);

        if (doc.HasMember("propertyValues"))
        {
            auto& props = doc["propertyValues"];
            if (props.HasMember("baseColor.color") && props["baseColor.color"].IsArray())
            {
                auto& arr = props["baseColor.color"];
                auto& node = CreateNode(NodeType::ConstantColor, "Base Color", pos);
                node.FloatValues[0] = arr[0].GetFloat();
                node.FloatValues[1] = arr[1].GetFloat();
                node.FloatValues[2] = arr[2].GetFloat();
                node.FloatValues[3] = arr[3].GetFloat();
                pos.x += 250.0f;

                Link link;
                link.Id = ed::LinkId(GetNextLinkId());
                link.StartPinId = node.Outputs[0].Id;
                link.EndPinId = output.Inputs[0].Id;
                s_links.push_back(link);
            }

            if (props.HasMember("baseColor.textureMap") && props["baseColor.textureMap"].IsString())
            {
                auto& node = CreateNode(NodeType::TextureSample, "Base Map", pos);
                node.TexturePath = props["baseColor.textureMap"].GetString();
                pos.x += 250.0f;
            }

            if (props.HasMember("roughness.factor") && props["roughness.factor"].IsNumber())
            {
                auto& node = CreateNode(NodeType::ConstantFloat, "Roughness", pos);
                node.FloatValues[0] = props["roughness.factor"].GetFloat();
                pos.x += 250.0f;

                Link link;
                link.Id = ed::LinkId(GetNextLinkId());
                link.StartPinId = node.Outputs[0].Id;
                link.EndPinId = output.Inputs[2].Id;
                s_links.push_back(link);
            }

            if (props.HasMember("opacity.factor") && props["opacity.factor"].IsNumber())
            {
                auto& node = CreateNode(NodeType::ConstantFloat, "Opacity", pos);
                node.FloatValues[0] = props["opacity.factor"].GetFloat();
                pos.x += 250.0f;

                Link link;
                link.Id = ed::LinkId(GetNextLinkId());
                link.StartPinId = node.Outputs[0].Id;
                link.EndPinId = output.Inputs[5].Id;
                s_links.push_back(link);
            }
        }
    }

    void SaveMaterial(const char* path)
    {
        MaterialNode* outputNode = nullptr;
        for (auto& node : s_nodes)
        {
            if (node.Type == NodeType::Output)
            {
                outputNode = &node;
                break;
            }
        }
        if (!outputNode)
            return;

        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();
        doc.AddMember("materialType", "@gemroot:Atom_Feature_Common@/Assets/Materials/Types/StandardPBR.materialtype", alloc);
        doc.AddMember("materialTypeVersion", 5, alloc);

        rapidjson::Value propertyValues(rapidjson::kObjectType);

        for (auto& pin : outputNode->Inputs)
        {
            for (auto& link : s_links)
            {
                if (link.EndPinId != pin.Id)
                    continue;

                MaterialNode& sourceNode = FindNode(ed::NodeId(FindPin(link.StartPinId).NodeId));
                if (pin.Name == "BaseColor" && sourceNode.Type == NodeType::ConstantColor)
                {
                    rapidjson::Value color(rapidjson::kArrayType);
                    color.PushBack(sourceNode.FloatValues[0], alloc);
                    color.PushBack(sourceNode.FloatValues[1], alloc);
                    color.PushBack(sourceNode.FloatValues[2], alloc);
                    color.PushBack(sourceNode.FloatValues[3], alloc);
                    propertyValues.AddMember("baseColor.color", color, alloc);
                }
                else if (pin.Name == "Roughness" && sourceNode.Type == NodeType::ConstantFloat)
                {
                    propertyValues.AddMember("roughness.factor", sourceNode.FloatValues[0], alloc);
                }
                else if (pin.Name == "Opacity" && sourceNode.Type == NodeType::ConstantFloat)
                {
                    propertyValues.AddMember("opacity.factor", sourceNode.FloatValues[0], alloc);
                }
            }
        }

        doc.AddMember("propertyValues", propertyValues, alloc);

        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
            return;

        AZ::IO::FileIOStream file(path, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath);
        if (file.IsOpen())
        {
            file.Write(buffer.GetSize(), buffer.GetString());
            file.Close();
        }
        s_currentMaterialPath = path;
    }

    void NewMaterial()
    {
        s_nodes.clear();
        s_links.clear();
        s_nextNodeId = 1;
        s_nextPinId = 1;
        s_nextLinkId = 1;
        s_currentMaterialPath.clear();

        CreateNode(NodeType::Output, "Output", ImVec2(450, 250));
        auto& baseColor = CreateNode(NodeType::ConstantColor, "Base Color", ImVec2(100, 150));
        baseColor.FloatValues[0] = 0.5f;
        baseColor.FloatValues[1] = 0.5f;
        baseColor.FloatValues[2] = 0.5f;
        baseColor.FloatValues[3] = 1.0f;
    }

    const char* GetCurrentMaterialPath()
    {
        return s_currentMaterialPath.empty() ? nullptr : s_currentMaterialPath.c_str();
    }

    void Render()
    {
        if (!s_editorContext)
            return;

        ed::SetCurrentEditor(s_editorContext);

        ImVec2 availSize = ImGui::GetContentRegionAvail();
        ImGui::SetNextWindowSize(availSize, ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

        static bool dummyOpen = true;
        ImGui::Begin("Material Editor##ImGuiMaterial", &dummyOpen,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New"))
                    NewMaterial();
                if (ImGui::MenuItem("Save"))
                {
                    if (!s_currentMaterialPath.empty())
                        SaveMaterial(s_currentMaterialPath.c_str());
                }
                if (ImGui::MenuItem("Save As..."))
                    ImGui::OpenPopup("Save Material As");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Delete Selected", "Del"))
                {
                    if (s_selectedNodeId)
                    {
                        s_links.erase(
                            std::remove_if(s_links.begin(), s_links.end(),
                                [](const Link& link) {
                                    return FindPin(link.StartPinId).NodeId == s_selectedNodeId.Get()
                                        || FindPin(link.EndPinId).NodeId == s_selectedNodeId.Get();
                                }),
                            s_links.end()
                        );
                        s_nodes.erase(
                            std::remove_if(s_nodes.begin(), s_nodes.end(),
                                [](const MaterialNode& n) { return n.Id == s_selectedNodeId; }),
                            s_nodes.end()
                        );
                        s_selectedNodeId = ed::NodeId(0);
                        s_hasSelection = false;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (ImGui::BeginPopupModal("Save Material As", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static char pathBuf[512] = {};
            ImGui::SetNextItemWidth(400.0f);
            ImGui::InputText("Path", pathBuf, sizeof(pathBuf));
            if (ImGui::Button("Save", ImVec2(120, 0)))
            {
                if (strlen(pathBuf) > 0)
                {
                    SaveMaterial(pathBuf);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::PushStyleColor(ImGuiCol_Text, O3DE_ACCENT_GREEN);
        if (s_currentMaterialPath.empty())
            ImGui::Text("MATERIAL EDITOR - Untitled");
        else
            ImGui::Text("MATERIAL EDITOR - %s", s_currentMaterialPath.c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();

        float panelWidth = ImGui::GetContentRegionAvail().x * 0.8f;
        ed::Begin("NodeCanvas", ImVec2(panelWidth, ImGui::GetContentRegionAvail().y));

        for (auto& node : s_nodes)
            ed::SetNodePosition(node.Id, node.Position);

        for (auto& node : s_nodes)
            RenderNode(node);

        for (auto& link : s_links)
            ed::Link(link.Id, link.StartPinId, link.EndPinId);

        if (ed::BeginCreate())
        {
            ed::PinId startPinId, endPinId;
            if (ed::QueryNewLink(&startPinId, &endPinId))
            {
                Pin& startPin = FindPin(startPinId);
                Pin& endPin = FindPin(endPinId);

                if (startPin.IsInput && !endPin.IsInput)
                {
                    std::swap(startPinId, endPinId);
                    std::swap(startPin, endPin);
                }

                if (CanCreateLink(startPin, endPin))
                {
                    if (ed::AcceptNewItem())
                    {
                        Link link;
                        link.Id = ed::LinkId(GetNextLinkId());
                        link.StartPinId = startPinId;
                        link.EndPinId = endPinId;
                        s_links.push_back(link);
                    }
                }
                else
                {
                    ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                }
            }
        }
        ed::EndCreate();

        if (ed::BeginDelete())
        {
            ed::LinkId deletedLinkId;
            while (ed::QueryDeletedLink(&deletedLinkId))
            {
                s_links.erase(
                    std::remove_if(s_links.begin(), s_links.end(),
                        [&](const Link& link) { return link.Id == deletedLinkId; }),
                    s_links.end()
                );
            }
            ed::NodeId deletedNodeId;
            while (ed::QueryDeletedNode(&deletedNodeId))
            {
                s_links.erase(
                    std::remove_if(s_links.begin(), s_links.end(),
                        [&](const Link& link) {
                            return FindPin(link.StartPinId).NodeId == deletedNodeId.Get()
                                || FindPin(link.EndPinId).NodeId == deletedNodeId.Get();
                        }),
                    s_links.end()
                );
                s_nodes.erase(
                    std::remove_if(s_nodes.begin(), s_nodes.end(),
                        [&](const MaterialNode& n) { return n.Id == deletedNodeId; }),
                    s_nodes.end()
                );
            }
        }
        ed::EndDelete();

        ed::Suspend();
        ed::NodeId contextNodeId;
        if (ed::ShowNodeContextMenu(&contextNodeId))
        {
            ImGui::OpenPopup("NodeContextMenu");
            s_selectedNodeId = contextNodeId;
        }
        else if (ed::ShowBackgroundContextMenu())
        {
            ImGui::OpenPopup("CanvasContextMenu");
        }
        ed::Resume();

        if (ImGui::BeginPopup("NodeContextMenu"))
        {
            ImGui::TextColored(O3DE_ACCENT_GREEN, "Node Options");
            ImGui::Separator();
            if (ImGui::MenuItem("Delete Node"))
            {
                ed::NodeId id = s_selectedNodeId;
                s_links.erase(
                    std::remove_if(s_links.begin(), s_links.end(),
                        [&](const Link& link) {
                            return FindPin(link.StartPinId).NodeId == id.Get()
                                || FindPin(link.EndPinId).NodeId == id.Get();
                        }),
                    s_links.end()
                );
                s_nodes.erase(
                    std::remove_if(s_nodes.begin(), s_nodes.end(),
                        [&](const MaterialNode& n) { return n.Id == id; }),
                    s_nodes.end()
                );
                s_selectedNodeId = ed::NodeId(0);
                s_hasSelection = false;
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("CanvasContextMenu"))
        {
            ImGui::TextColored(O3DE_ACCENT_GREEN, "Add Node");
            ImGui::Separator();
            if (ImGui::BeginMenu("Constants"))
            {
                if (ImGui::MenuItem("Float"))
                    CreateNode(NodeType::ConstantFloat, "Float", ed::ScreenToCanvas(ImGui::GetMousePos()));
                if (ImGui::MenuItem("Color"))
                    CreateNode(NodeType::ConstantColor, "Color", ed::ScreenToCanvas(ImGui::GetMousePos()));
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Textures"))
            {
                if (ImGui::MenuItem("Texture Sample"))
                    CreateNode(NodeType::TextureSample, "Sample", ed::ScreenToCanvas(ImGui::GetMousePos()));
                if (ImGui::MenuItem("Texture Coordinate"))
                    CreateNode(NodeType::TextureCoordinate, "UV", ed::ScreenToCanvas(ImGui::GetMousePos()));
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Math"))
            {
                if (ImGui::MenuItem("Add"))
                    CreateNode(NodeType::MathAdd, "Add", ed::ScreenToCanvas(ImGui::GetMousePos()));
                if (ImGui::MenuItem("Multiply"))
                    CreateNode(NodeType::MathMultiply, "Mul", ed::ScreenToCanvas(ImGui::GetMousePos()));
                if (ImGui::MenuItem("Lerp"))
                    CreateNode(NodeType::MathLerp, "Lerp", ed::ScreenToCanvas(ImGui::GetMousePos()));
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Material Output"))
                CreateNode(NodeType::Output, "Output", ed::ScreenToCanvas(ImGui::GetMousePos()));
            ImGui::EndPopup();
        }

        s_hasSelection = false;
        s_selectedNodeId = ed::NodeId(0);
        int selectedCount = ed::GetSelectedObjectCount();
        if (selectedCount > 0)
        {
            ed::NodeId nodes[1];
            if (ed::GetSelectedNodes(nodes, 1) > 0)
            {
                s_selectedNodeId = nodes[0];
                s_hasSelection = true;
            }
        }

        ed::End();

        ImGui::SameLine();
        ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
        RenderRightPanel();
        ImGui::EndChild();

        ed::SetCurrentEditor(nullptr);
        ImGui::End();
    }
}