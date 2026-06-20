#pragma once

namespace ImGuiMaterialNodeEditor
{
    void Initialize();
    void Shutdown();
    void Render();
    void LoadMaterial(const char* path);
    void SaveMaterial(const char* path);
    void NewMaterial();
    const char* GetCurrentMaterialPath();
}