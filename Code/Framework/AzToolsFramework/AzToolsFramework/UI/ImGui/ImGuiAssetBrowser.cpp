#include "EditorDefs.h"
#include "ImGuiAssetBrowser.h"

#include <imgui/imgui.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <algorithm>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace ImGuiAssetBrowser
{
    // O3DE theme colors
    static const ImVec4 O3DE_ACCENT_GREEN = ImVec4(0.29f, 0.48f, 0.30f, 1.0f);
    static const ImVec4 O3DE_TEXT_NORMAL = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    static const ImVec4 O3DE_TEXT_MUTED = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    static const ImVec4 O3DE_BG_HOVER = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
    static const ImVec4 O3DE_FOLDER_COLOR = ImVec4(0.8f, 0.7f, 0.3f, 1.0f);
    static const ImVec4 O3DE_FILE_COLOR = ImVec4(0.7f, 0.7f, 0.9f, 1.0f);
    static const ImVec4 O3DE_SELECTED_BG = ImVec4(0.22f, 0.35f, 0.22f, 1.0f);

    static char s_searchBuffer[256] = "";
    static AZStd::string s_currentPath;
    static AZStd::string s_selectedAssetPath;
    static AZStd::string s_selectedAssetName;

    // File entry info
    struct AssetEntry
    {
        AZStd::string name;
        AZStd::string fullPath;
        bool isDirectory;
        AZ::u64 fileSize;
        AZStd::string extension;
    };

    static AZStd::vector<AssetEntry> s_currentEntries;
    static AZStd::vector<AZStd::string> s_breadcrumbPath;
    static bool s_needsRefresh = true;
    static bool s_showFolders = true;
    static int s_filterType = 0; // 0=All, 1=Meshes, 2=Textures, 3=Materials, 4=Prefabs, 5=Scripts

    static const char* s_filterNames[] = {
        "All", "Meshes", "Textures", "Materials", "Prefabs", "Scripts"
    };

    static const char* s_filterExtensions[] = {
        "", ".fbx;.obj;.gltf;.glb;.stl", ".png;.jpg;.tga;.dds;.bmp;.tif;.exr;.psd",
        ".material;.mat", ".prefab;.slice", ".lua;.py;.scriptcanvas;.script"
    };

    void Initialize()
    {
        memset(s_searchBuffer, 0, sizeof(s_searchBuffer));
        s_currentPath.clear();
        s_selectedAssetPath.clear();
        s_selectedAssetName.clear();
        s_currentEntries.clear();
        s_breadcrumbPath.clear();
        s_needsRefresh = true;
    }

    void Shutdown()
    {
        s_currentEntries.clear();
        s_breadcrumbPath.clear();
    }

    const char* GetSelectedAssetPath()
    {
        return s_selectedAssetPath.empty() ? nullptr : s_selectedAssetPath.c_str();
    }

    void ClearSelectedAsset()
    {
        s_selectedAssetPath.clear();
        s_selectedAssetName.clear();
    }

    // Split string by delimiter
    static AZStd::vector<AZStd::string> SplitString(const AZStd::string& str, char delimiter)
    {
        AZStd::vector<AZStd::string> parts;
        size_t start = 0;
        size_t end = str.find(delimiter);
        while (end != AZStd::string::npos)
        {
            if (end > start)
                parts.push_back(str.substr(start, end - start));
            start = end + 1;
            end = str.find(delimiter, start);
        }
        if (start < str.length())
            parts.push_back(str.substr(start));
        return parts;
    }

    // Check file extension against a semicolon-separated list
    static bool MatchesExtension(const AZStd::string& ext, const AZStd::string& filterExts)
    {
        if (filterExts.empty())
            return true;
        AZStd::vector<AZStd::string> exts = SplitString(filterExts, ';');
        for (const AZStd::string& filterExt : exts)
        {
            // Trim whitespace
            AZStd::string trimmed = filterExt;
            trimmed.erase(0, trimmed.find_first_not_of(" "));
            trimmed.erase(trimmed.find_last_not_of(" ") + 1);
            if (azstricmp(ext.c_str(), trimmed.c_str()) == 0)
                return true;
        }
        return false;
    }

    // Get file extension in lowercase
    static AZStd::string GetExtension(const AZStd::string& filename)
    {
        size_t dotPos = filename.rfind('.');
        if (dotPos == AZStd::string::npos)
            return "";
        AZStd::string ext = filename.substr(dotPos);
        AZStd::to_lower(ext.begin(), ext.end());
        return ext;
    }

    // Scan directory using FileIOBase
    static void ScanDirectory(const AZStd::string& path)
    {
        s_currentEntries.clear();

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
            return;

        // Fix path format - ensure it ends with /
        AZStd::string searchPath = path;
        if (!searchPath.empty() && searchPath.back() != '/' && searchPath.back() != '\\')
            searchPath += '/';
        searchPath += "*";

        // Find all files and directories
        fileIO->FindFiles(path.c_str(), "*", [fileIO](const char* fullPath) -> bool
        {
            if (!fullPath)
                return true;

            AZStd::string fullPathStr(fullPath);
            
            // Skip self (. and ..)
            AZStd::string filename;
            size_t lastSep = fullPathStr.rfind('/');
            if (lastSep != AZStd::string::npos)
                filename = fullPathStr.substr(lastSep + 1);
            else
                filename = fullPathStr;

            if (filename == "." || filename == "..")
                return true;

            AssetEntry entry;
            entry.name = filename;
            entry.fullPath = fullPathStr;
            entry.isDirectory = fileIO->IsDirectory(fullPathStr.c_str());
            entry.fileSize = 0;
            entry.extension = GetExtension(filename);

            // Get file size for non-directories
            if (!entry.isDirectory)
            {
                AZ::IO::FileIOStream fileStream(fullPathStr.c_str(), AZ::IO::OpenMode::ModeRead);
                if (fileStream.IsOpen())
                {
                    entry.fileSize = fileStream.GetLength();
                    fileStream.Close();
                }
            }

            s_currentEntries.push_back(entry);
            return true;
        });

        // Sort: directories first, then files, alphabetically
        std::sort(s_currentEntries.begin(), s_currentEntries.end(),
            [](const AssetEntry& a, const AssetEntry& b)
            {
                if (a.isDirectory != b.isDirectory)
                    return a.isDirectory; // directories first
                return azstricmp(a.name.c_str(), b.name.c_str()) < 0;
            });

        // Update breadcrumbs
        s_breadcrumbPath = SplitString(path, '/');
    }

    // Navigate to a subdirectory
    static void NavigateTo(const AZStd::string& subDir)
    {
        if (s_currentPath.empty())
            s_currentPath = subDir;
        else if (subDir == "..")
        {
            // Go up one level
            size_t lastSep = s_currentPath.rfind('/');
            if (lastSep != AZStd::string::npos)
                s_currentPath = s_currentPath.substr(0, lastSep);
            if (s_currentPath.empty())
                s_currentPath = "@projectroot@";
        }
        else
        {
            if (!s_currentPath.empty() && s_currentPath.back() != '/')
                s_currentPath += '/';
            s_currentPath += subDir;
        }
        s_needsRefresh = true;
        s_selectedAssetPath.clear();
        s_selectedAssetName.clear();
    }

    // Get root asset paths
    static AZStd::vector<AZStd::string> GetRootPaths()
    {
        AZStd::vector<AZStd::string> roots;
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
            return roots;

        // Try common root aliases
        const char* aliases[] = { "@projectroot@/Assets", "@assets@", "@devassets@", "@root@/Assets" };
        for (const char* alias : aliases)
        {
            if (fileIO->Exists(alias))
            {
                char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
                if (fileIO->ResolvePath(alias, resolvedPath, AZ_MAX_PATH_LEN))
                {
                    roots.push_back(resolvedPath);
                }
            }
        }

        return roots;
    }

    // Format file size
    static AZStd::string FormatFileSize(AZ::u64 bytes)
    {
        if (bytes < 1024)
            return AZStd::string::format("%llu B", bytes);
        if (bytes < 1024 * 1024)
            return AZStd::string::format("%.1f KB", bytes / 1024.0f);
        if (bytes < 1024 * 1024 * 1024)
            return AZStd::string::format("%.1f MB", bytes / (1024.0f * 1024.0f));
        return AZStd::string::format("%.1f GB", bytes / (1024.0f * 1024.0f * 1024.0f));
    }

    // Get icon for file type
    static const char* GetFileIcon(const AssetEntry& entry)
    {
        if (entry.isDirectory)
            return "(DIR)";
        
        const AZStd::string& ext = entry.extension;
        if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".stl")
            return "(M)";
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".dds" ||
            ext == ".bmp" || ext == ".tif" || ext == ".exr" || ext == ".psd")
            return "(T)";
        if (ext == ".material" || ext == ".mat")
            return "(Mtl)";
        if (ext == ".prefab" || ext == ".slice")
            return "(P)";
        if (ext == ".lua" || ext == ".py" || ext == ".scriptcanvas" || ext == ".script")
            return "(S)";
        if (ext == ".azasset")
            return "(A)";
        if (ext == ".pass")
            return "(Ps)";
        return "(F)";
    }

    // Render breadcrumb navigation bar
    static void RenderBreadcrumbs()
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, O3DE_ACCENT_GREEN);

        // "Roots" button
        if (ImGui::Button("Roots"))
        {
            s_currentPath.clear();
            s_needsRefresh = true;
        }
        ImGui::SameLine();

        for (size_t i = 0; i < s_breadcrumbPath.size(); ++i)
        {
            ImGui::Text("/");
            ImGui::SameLine();
            
            AZStd::string label;
            if (s_breadcrumbPath[i].find("@") != AZStd::string::npos)
            {
                // Show simplified alias name
                size_t atPos = s_breadcrumbPath[i].find('@');
                label = s_breadcrumbPath[i].substr(1, atPos - 1);
            }
            else
            {
                label = s_breadcrumbPath[i];
            }

            char btnLabel[128];
            azsnprintf(btnLabel, sizeof(btnLabel), "%s##bc%zu", label.c_str(), i);

            if (ImGui::Button(btnLabel))
            {
                // Navigate to this breadcrumb level
                AZStd::string newPath;
                for (size_t j = 0; j <= i; ++j)
                {
                    if (j > 0) newPath += '/';
                    newPath += s_breadcrumbPath[j];
                }
                if (i > 0 && s_breadcrumbPath[0].find('@') != AZStd::string::npos)
                {
                    // Keep the full path
                    s_currentPath = newPath;
                }
                else if (i == 0)
                {
                    s_currentPath = s_breadcrumbPath[0];
                }
                else
                {
                    s_currentPath = newPath;
                }
                s_needsRefresh = true;
            }
            ImGui::SameLine();
        }

        ImGui::PopStyleColor(3);
        ImGui::NewLine();
    }

    void Render()
    {
        ImVec2 availSize = ImGui::GetContentRegionAvail();
        ImGui::SetNextWindowSize(availSize, ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

        static bool s_dummyClose = true;
        if (!ImGui::Begin("Asset Browser##ImGui", &s_dummyClose,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoBringToFrontOnFocus))
        {
            ImGui::End();
            return;
        }

        // Header
        ImGui::PushStyleColor(ImGuiCol_Text, O3DE_ACCENT_GREEN);
        ImGui::Text("ASSET BROWSER");
        ImGui::PopStyleColor();
        ImGui::Separator();

        // Toolbar row
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, O3DE_ACCENT_GREEN);

        // Back button
        if (ImGui::Button("<- Back"))
        {
            NavigateTo("..");
        }
        ImGui::SameLine();

        // Refresh button
        if (ImGui::Button("Refresh"))
        {
            s_needsRefresh = true;
        }
        ImGui::SameLine();

        // Filter dropdown
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::Combo("##Filter", &s_filterType, s_filterNames, IM_ARRAYSIZE(s_filterNames)))
        {
            s_needsRefresh = true;
        }

        ImGui::SameLine();

        // Search bar
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 10);
        ImGui::InputTextWithHint("##Search", "Search assets...", s_searchBuffer, IM_ARRAYSIZE(s_searchBuffer));

        ImGui::PopStyleColor(3);

        ImGui::Separator();

        // Breadcrumbs
        RenderBreadcrumbs();

        // Separator
        ImGui::Separator();

        // Scan directory if needed
        if (s_needsRefresh)
        {
            if (!s_currentPath.empty())
            {
                ScanDirectory(s_currentPath);
            }
            else
            {
                // Show root paths
                s_currentEntries.clear();
                auto roots = GetRootPaths();
                for (const auto& root : roots)
                {
                    AssetEntry entry;
                    entry.name = root;
                    entry.fullPath = root;
                    entry.isDirectory = true;
                    entry.fileSize = 0;
                    s_currentEntries.push_back(entry);
                }
                s_breadcrumbPath.clear();
            }
            s_needsRefresh = false;
        }

        // Files and directories list
        ImGui::BeginChild("AssetList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);

        if (s_currentEntries.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
            ImGui::Text("No assets found");
            ImGui::PopStyleColor();
        }
        else
        {
            // Column headers
            ImGui::Columns(3, "AssetColumns", false);
            ImGui::SetColumnWidth(0, ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::SetColumnWidth(1, ImGui::GetContentRegionAvail().x * 0.25f);
            ImGui::SetColumnWidth(2, ImGui::GetContentRegionAvail().x * 0.25f);

            ImGui::PushStyleColor(ImGuiCol_Text, O3DE_ACCENT_GREEN);
            ImGui::Text("Name");
            ImGui::NextColumn();
            ImGui::Text("Type");
            ImGui::NextColumn();
            ImGui::Text("Size");
            ImGui::NextColumn();
            ImGui::PopStyleColor();
            ImGui::Separator();

            for (const AssetEntry& entry : s_currentEntries)
            {
                // Apply search filter
                if (strlen(s_searchBuffer) > 0)
                {
                    AZStd::string searchLower = AZStd::string(s_searchBuffer);
                    AZStd::to_lower(searchLower.begin(), searchLower.end());
                    AZStd::string nameLower = entry.name;
                    AZStd::to_lower(nameLower.begin(), nameLower.end());
                    if (nameLower.find(searchLower) == AZStd::string::npos)
                        continue;
                }

                // Apply type filter (skip directories unless showing all)
                if (s_filterType > 0 && !entry.isDirectory)
                {
                    if (!MatchesExtension(entry.extension, s_filterExtensions[s_filterType]))
                        continue;
                }

                // Determine text color
                ImVec4 textColor = entry.isDirectory ? O3DE_FOLDER_COLOR : O3DE_FILE_COLOR;
                
                // Check if selected
                bool isSelected = (entry.fullPath == s_selectedAssetPath);

                // Row background for selection
                if (isSelected)
                {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(O3DE_SELECTED_BG));
                }

                // Clickable row
                ImGui::PushID(entry.fullPath.c_str());

                // Icon + Name
                ImGui::PushStyleColor(ImGuiCol_Text, textColor);
                const char* icon = GetFileIcon(entry);
                ImGui::Text("%s  %s", icon, entry.name.c_str());
                ImGui::PopStyleColor();

                // Handle click
                if (ImGui::IsItemClicked())
                {
                    if (entry.isDirectory)
                    {
                        NavigateTo(entry.name);
                    }
                    else
                    {
                        s_selectedAssetPath = entry.fullPath;
                        s_selectedAssetName = entry.name;
                    }
                }

                // Double-click to navigate/select
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    if (!entry.isDirectory)
                    {
                        // Open/select the asset
                        s_selectedAssetPath = entry.fullPath;
                        s_selectedAssetName = entry.name;
                    }
                }

                // Context menu
                if (ImGui::BeginPopupContextItem("AssetContextMenu"))
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, O3DE_ACCENT_GREEN);
                    ImGui::Text("%s", entry.name.c_str());
                    ImGui::PopStyleColor();
                    ImGui::Separator();

                    if (ImGui::MenuItem("Copy Path"))
                    {
                        ImGui::SetClipboardText(entry.fullPath.c_str());
                    }
                    if (!entry.isDirectory && ImGui::MenuItem("Select"))
                    {
                        s_selectedAssetPath = entry.fullPath;
                        s_selectedAssetName = entry.name;
                    }
                    ImGui::EndPopup();
                }

                ImGui::NextColumn();

                // Type column
                ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
                if (entry.isDirectory)
                    ImGui::Text("Folder");
                else
                    ImGui::Text("%s", entry.extension.c_str());
                ImGui::PopStyleColor();
                ImGui::NextColumn();

                // Size column
                ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);
                if (entry.isDirectory)
                    ImGui::Text("-");
                else
                    ImGui::Text("%s", FormatFileSize(entry.fileSize).c_str());
                ImGui::PopStyleColor();
                ImGui::NextColumn();

                ImGui::PopID();
            }

            ImGui::Columns(1);
        }

        ImGui::EndChild();

        // Status bar
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, O3DE_TEXT_MUTED);

        size_t visibleCount = 0;
        size_t filteredCount = 0;
        for (const auto& entry : s_currentEntries)
        {
            if (!entry.isDirectory) filteredCount++;
            if (strlen(s_searchBuffer) > 0)
            {
                AZStd::string searchLower = AZStd::string(s_searchBuffer);
                AZStd::to_lower(searchLower.begin(), searchLower.end());
                AZStd::string nameLower = entry.name;
                AZStd::to_lower(nameLower.begin(), nameLower.end());
                if (nameLower.find(searchLower) != AZStd::string::npos)
                    visibleCount++;
            }
            else
            {
                visibleCount++;
            }
        }

        AZStd::string statusText = AZStd::string::format(
            "Showing: %zu / %zu total | %s",
            visibleCount, filteredCount,
            s_selectedAssetName.empty() ? "No selection" : s_selectedAssetName.c_str()
        );
        ImGui::Text("%s", statusText.c_str());

        ImGui::PopStyleColor();

        ImGui::End();
    }
}