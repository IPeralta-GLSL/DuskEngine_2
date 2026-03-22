# Memory Leak Fixes

## Resultado medido

| Escena | Antes | Después | Reducción |
|---|---|---|---|
| Intel Sponza + Ray Tracing (sin nivel abierto) | ~12 GB | ~3.5 GB | **~71%** |

Medido en Dusk Engine Editor (Development Build), AMD Radeon RX 6700, Vulkan, 22 de marzo de 2026.

---

## 1. LY-115607 — `AZStd::map` node leak on hint insert

**File:** `Code/Framework/AzCore/AzCore/std/containers/rbtree.h`

**Affected overloads:**
- `insert_unique(const_iterator, value_type&&)`
- `insert_unique(const_iterator, const value_type&)`
- `emplace_unique(const_iterator, InputArguments&&...)`

**Root cause:** When inserting a duplicate key with a hint, a new tree node was allocated before knowing whether the key already existed. The node was only freed if `result == insertPos` — i.e. if the returned iterator happened to equal the hint. When the duplicate existed but the hint did not point at it, the condition was false and the node was silently abandoned. One node leaked per such call.

A pre-existing workaround was already documented in `Gems/Atom/RHI/Code/Include/Atom/RHI/interval_map.h`:
```cpp
// Using std::map for now because AZStd::map has a memory leak when inserting with a hint (LY-115607)
```

**Fix:** Replace the iterator comparison with a node pointer comparison. If the returned iterator does not point to the newly allocated node, the insert was rejected and the node must be freed:
```cpp
// Before
if (result == insertPos) { deallocate_node(newNode); }

// After
const bool wasInserted = (result.m_node == newNode);
if (!wasInserted) { deallocate_node(newNode); }
```

---

## 2. `ShaderVariantAsyncLoader` — stale reverse-lookup entry on error

**File:** `Gems/Atom/RPI/Code/Source/RPI.Public/Shader/ShaderVariantAsyncLoader.cpp`  
**Function:** `OnShaderVariantTreeAssetError`

**Root cause:** On a failed shader variant tree load, `m_shaderVariantData` was erased correctly but the corresponding entry in `m_shaderAssetIdToShaderVariantTreeAssetId` was left behind. The stale entry persisted indefinitely and could cause incorrect lookups if the same shader asset was registered again.

**Fix:** Erase from both maps atomically within the same lock scope:
```cpp
m_shaderVariantData.erase(findIt);
m_shaderAssetIdToShaderVariantTreeAssetId.erase(shaderAssetId);
```

---

## 3. `ModelReloaderSystem` — in-flight reloaders leaked on shutdown

**Files:** `Gems/Atom/Feature/Common/Code/Source/Mesh/ModelReloaderSystem.h` / `.cpp`

**Root cause:** `ModelReloader` objects are heap-allocated with `new` and stored in `m_pendingReloads`. Each reloader normally deletes itself via `delete this` after signalling completion. However, `ModelReloaderSystem` had no destructor — any reload still in-flight when the system shuts down (e.g. editor close during a hot-reload or rapid asset reimport) leaked the entire reloader, including its asset references, dependency lists, and event handlers.

**Fix:** Added a destructor that acquires the mutex and explicitly deletes all remaining reloader pointers before clearing the map:
```cpp
ModelReloaderSystem::~ModelReloaderSystem()
{
    AZStd::scoped_lock lock(m_pendingReloadMutex);
    for (auto& [assetId, reloader] : m_pendingReloads)
    {
        delete reloader;
    }
    m_pendingReloads.clear();
}
```

---

## 4. `BehaviorEBusEventSender` — methods leaked on duplicate event reflection

**File:** `Code/Framework/AzCore/AzCore/RTTI/BehaviorEBusBuilder.inl`  
**Function:** `EBusBuilderBase::EventWithBus`

**Root cause:** `BehaviorEBusEventSender::Set<Bus>()` allocates up to four `BehaviorMethod*` objects via `aznew` (`m_broadcast`, `m_event`, `m_queueBroadcast`, `m_queueEvent`). When a gem reflects the same EBus event twice, the map insert fails and the local `ebusEvent` goes out of scope — but the struct has no destructor, so all allocated `BehaviorMethod` pointers are abandoned. Only an `AZ_Error` was emitted with no cleanup.

**Fix:** After the failed insert, explicitly delete all four method pointers:
```cpp
if (!insertIt.second)
{
    AZ_Error(...);
    delete ebusEvent.m_broadcast;
    delete ebusEvent.m_event;
    delete ebusEvent.m_queueBroadcast;
    delete ebusEvent.m_queueEvent;
}
```

---

## 5. `StreamingImagePool::AllocateImageTilesInternal` — assert-only guard on tile overwrite (DX12)

**File:** `Gems/Atom/RHI/DX12/Code/Source/RHI/StreamingImagePool.cpp`  
**Function:** `AllocateImageTilesInternal`

**Root cause:** If `AllocateImageTilesInternal` was called when `m_heapTiles[subresourceIndex]` was already populated, an `AZ_Assert` fired in Debug builds. In Release, the assert is stripped and the existing GPU heap-tile allocation was silently overwritten, leaking the old tiles permanently.

**Fix:** Added an explicit early `return` immediately after the assert so the overwrite cannot occur in any build configuration:
```cpp
AZ_Assert(image.m_heapTiles[subresourceIndex].size() == 0, "Stopping on an existing tile allocation. This will leak.");
if (image.m_heapTiles[subresourceIndex].size() != 0)
{
    return;
}
```

---

## 6. `AssetManager::SetInstance` — previous instance leaked in Release builds

**File:** `Code/Framework/AzCore/AzCore/Asset/AssetManager.cpp`  
**Function:** `AssetManager::SetInstance`

**Root cause:** `SetInstance` had an `AZ_Assert` that warned calling it while a prior instance existed would leak the old `AssetManager`. In Release builds the assert is stripped, so the raw pointer assignment silently replaced the previous instance, leaking all of its assets, job queues, and registry data.

**Fix:** Added an explicit guard before the assignment that refuses the call and returns `false` in all build configurations:
```cpp
if (*s_assetDB)
{
    return false;
}
(*s_assetDB) = assetManager;
```


**Files:** `Gems/Atom/Feature/Common/Code/Source/Mesh/ModelReloaderSystem.h` / `.cpp`

**Root cause:** `ModelReloader` objects are allocated with `new` and stored in `m_pendingReloads`. Each reloader deletes itself via `delete this` after firing its completion event. However, `ModelReloaderSystem` had no destructor, so any reload still in-flight when the system shuts down (e.g. rapid asset reimport, editor close during hot-reload) was permanently leaked.

**Fix:** Add a destructor to `ModelReloaderSystem` that acquires the mutex and explicitly deletes all remaining reloader pointers before clearing the map.

---

## 4. `BehaviorEBusEventSender` — methods leaked on duplicate event reflection

**File:** `Code/Framework/AzCore/AzCore/RTTI/BehaviorEBusBuilder.inl`  
**Function:** `EBusBuilderBase::EventWithBus`

**Root cause:** `BehaviorEBusEventSender::Set<Bus>()` allocates up to four `BehaviorMethod*` objects via `aznew`. When a gem reflects the same EBus event twice, the map insert fails and the local `ebusEvent` goes out of scope — but the struct has no destructor, so all four allocated `BehaviorMethod` pointers leak. Only an `AZ_Error` was emitted with no cleanup.

**Fix:** After the failed insert, explicitly delete all four method pointers from the rejected `ebusEvent`:
```cpp
delete ebusEvent.m_broadcast;
delete ebusEvent.m_event;
delete ebusEvent.m_queueBroadcast;
delete ebusEvent.m_queueEvent;
```

---

## 5. `StreamingImagePool::AllocateImageTilesInternal` — assert-only guard on tile overwrite (DX12)

**File:** `Gems/Atom/RHI/DX12/Code/Source/RHI/StreamingImagePool.cpp`  
**Function:** `AllocateImageTilesInternal`

**Root cause:** If `AllocateImageTilesInternal` was called when `m_heapTiles[subresourceIndex]` was already populated, an `AZ_Assert` fired in Debug, but in Release the existing heap-tile allocation was silently overwritten, leaking the GPU tile memory.

**Fix:** Added an explicit early `return` after the assert so the overwrite is prevented in all build configurations.

---

## 6. `AssetManager::SetInstance` — previous instance leaked in Release builds

**File:** `Code/Framework/AzCore/AzCore/Asset/AssetManager.cpp`  
**Function:** `AssetManager::SetInstance`

**Root cause:** `SetInstance` had an `AZ_Assert` warning that calling it while a prior instance existed would leak the old `AssetManager`. In Release builds the assert is stripped and the raw pointer assignment silently replaced the old instance, leaking all assets, job queues, and registry data it owned.

**Fix:** Added an explicit `if (*s_assetDB) { return false; }` guard before the assignment so the function safely refuses the call in all build configurations.

---

## 1. LY-115607 — `AZStd::map` node leak on hint insert

**Files:** `Code/Framework/AzCore/AzCore/std/containers/rbtree.h`

**Affected overloads:**
- `insert_unique(const_iterator, value_type&&)`
- `insert_unique(const_iterator, const value_type&)`
- `emplace_unique(const_iterator, InputArguments&&...)`

**Root cause:** When inserting a duplicate key with a hint, a new node was allocated before knowing whether the key already existed. The node was only freed if `result == insertPos`, i.e. if the returned iterator happened to equal the hint. When the hint did not point at the existing duplicate node, the condition was false and the allocated node was silently abandoned.

**Fix:** Replace the iterator comparison with a node pointer comparison:
```cpp
// Before
if (result == insertPos) { deallocate_node(newNode); }

// After
const bool wasInserted = (result.m_node == newNode);  // or result.get_iterator().m_node with checked iterators
if (!wasInserted) { deallocate_node(newNode); }
```

A pre-existing workaround was already present in `Gems/Atom/RHI/Code/Include/Atom/RHI/interval_map.h`:
```cpp
// Using std::map for now because AZStd::map has a memory leak when inserting with a hint (LY-115607)
```

---

## 2. `ShaderVariantAsyncLoader` — stale reverse-lookup entry on error

**File:** `Gems/Atom/RPI/Code/Source/RPI.Public/Shader/ShaderVariantAsyncLoader.cpp`  
**Function:** `OnShaderVariantTreeAssetError`

**Root cause:** On a failed shader variant tree load, `m_shaderVariantData` was erased correctly but the corresponding entry in `m_shaderAssetIdToShaderVariantTreeAssetId` was left behind. That stale entry would persist indefinitely and cause incorrect lookups if the same shader asset key was registered again.

**Fix:** Erase from both maps atomically within the same lock scope:
```cpp
m_shaderVariantData.erase(findIt);
m_shaderAssetIdToShaderVariantTreeAssetId.erase(shaderAssetId);
```
