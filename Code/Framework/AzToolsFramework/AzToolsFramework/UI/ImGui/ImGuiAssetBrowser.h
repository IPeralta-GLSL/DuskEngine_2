#pragma once

#include <AzCore/Component/EntityId.h>

namespace ImGuiAssetBrowser
{
    // Initialize and shutdown
    void Initialize();
    void Shutdown();

    // Main render function
    void Render();

    // Get selected asset path
    const char* GetSelectedAssetPath();
    void ClearSelectedAsset();
}