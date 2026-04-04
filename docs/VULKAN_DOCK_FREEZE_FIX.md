# Fix: Viewport 3D Freeze on Dock/Resize (Linux + Vulkan)

## Síntoma

Al arrastrar, cerrar o redimensionar ventanas dock en el Editor de O3DE sobre Linux con el backend Vulkan, el visor 3D se congelaba permanentemente mientras la UI seguía respondiendo.

## Causa Raíz

El problema tenía dos componentes independientes:

### 1. `VK_SUBOPTIMAL_KHR` tratado como error fatal

`AcquireNextImageKHR` devuelve `VK_SUBOPTIMAL_KHR` (`1000001003`) cuando la superficie del dock cambia de tamaño o se reparenta. Este es un **código de éxito parcial** en Vulkan: la imagen está disponible y es usable, pero el swapchain debería recrearse pronto.

El problema es que `ConvertResult()` (el conversor de `VkResult` a `RHI::ResultCode`) solo reconoce `VK_SUCCESS` (0) como éxito. Cualquier otro valor, incluyendo `VK_SUBOPTIMAL_KHR`, devolvía `RHI::ResultCode::Fail`, deteniendo el render loop permanentemente.

### 2. `SurfaceAboutToBeDestroyed` destruía el contexto 3D irreversiblemente

Cuando Qt reparenta un widget dock (por ejemplo al arrastrarlo), envía un evento `QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed` de forma **transitoria** — la superficie se destruye temporalmente pero se recrea inmediatamente después en el nuevo padre.

El código original de `RenderViewportWidget::event()` mapeaba ese evento a `SendWindowCloseEvent()`, que destruía el contexto Vulkan del viewport de forma permanente, sin posibilidad de recuperación.

## Archivos Modificados

> **Nota sobre logs de debug**: Los cambios incluyen `AZ_Printf` temporales en `ProcessRecreation`, `ResizeInternal` y `WindowContext.cpp`. Pueden eliminarse en producción una vez validado el fix.

### `Gems/Atom/RHI/Vulkan/Code/Source/RHI/SwapChain.cpp`

**Función `ProcessRecreation`**: Agregados logs de debug para trazar el ciclo de recreación:

```cpp
AZ_Printf("WindowContext", "[DEBUG_VULKAN] ProcessRecreation: started (W:%d H:%d)\n", ...);
// ... recreación ...
AZ_Printf("WindowContext", "[DEBUG_VULKAN] ProcessRecreation: completed OK\n");
```

**Función `ResizeInternal`**: Agregado log de debug:

```cpp
AZ_Printf("WindowContext", "[DEBUG_VULKAN] ResizeInternal to W:%d H:%d\n", ...);
```

**Función `AcquireNewImage`**: Manejo correcto de todos los códigos de retorno de `vkAcquireNextImageKHR`:

| Código Vulkan | Comportamiento anterior | Comportamiento corregido |
|---|---|---|
| `VK_SUBOPTIMAL_KHR` | `ConvertResult` → `Fail` → render loop muerto | Normalizar a `VK_SUCCESS`, programar recreación, continuar renderizando |
| `VK_ERROR_OUT_OF_DATE_KHR` | `ConvertResult` → `Fail` sin recreación | Marcar `pendingRecreation = true`, saltear frame, liberar semáforo |
| `VK_TIMEOUT` / `VK_NOT_READY` | Timeout infinito con `UINT64_MAX` | Marcar `pendingRecreation = true`, saltear frame, liberar semáforo |
| `UINT64_MAX` timeout | Bloqueaba el thread principal indefinidamente | Cambiado a timeout de 100ms |
| Falla inesperada | `RETURN_RESULT_IF_UNSUCCESSFUL` sin liberar semáforo | Liberar semáforo antes de retornar error |

```cpp
// ANTES:
VkResult vkResult = device.GetContext().AcquireNextImageKHR(
    ..., UINT64_MAX, ...);
RHI::ResultCode result = ConvertResult(vkResult);
RETURN_RESULT_IF_UNSUCCESSFUL(result);  // VK_SUBOPTIMAL_KHR → Fail aquí

// DESPUÉS:
constexpr uint64_t timeoutNs = 100'000'000; // 100 ms
VkResult vkResult = device.GetContext().AcquireNextImageKHR(
    ..., timeoutNs, ...);

// Códigos que impiden usar la imagen → saltear frame
if (vkResult == VK_TIMEOUT || vkResult == VK_NOT_READY || vkResult == VK_ERROR_OUT_OF_DATE_KHR)
{
    semaphoreAllocator.DeAllocate(imageAvailableSemaphore);
    m_pendingRecreation = true;
    return RHI::ResultCode::Fail;
}

// VK_SUBOPTIMAL_KHR: imagen válida → normalizar antes de ConvertResult()
if (vkResult == VK_SUBOPTIMAL_KHR)
{
#if AZ_TRAIT_ATOM_VULKAN_RECREATE_SWAPCHAIN_WHEN_SUBOPTIMAL
    m_pendingRecreation = true;  // en Linux este trait = 1
#endif
    vkResult = VK_SUCCESS; // <-- clave: ConvertResult() no maneja VK_SUBOPTIMAL_KHR
}

RHI::ResultCode result = ConvertResult(vkResult);
if (result != RHI::ResultCode::Success)
{
    semaphoreAllocator.DeAllocate(imageAvailableSemaphore); // evitar leak
    return result;
}
```

### `Gems/Atom/RPI/Code/Source/RPI.Public/WindowContext.cpp`

Agregados logs de debug temporales en `OnWindowResized` y `CheckResizeSwapChain` para trazar el flujo de resize del swapchain:

```cpp
void WindowContext::OnWindowResized(uint32_t width, uint32_t height)
{
    AZ_Printf("WindowContext", "[DEBUG_RPI] OnWindowResized W:%d H:%d\n", width, height);
    CheckResizeSwapChain();
}

bool WindowContext::CheckResizeSwapChain()
{
    AZ_Printf("WindowContext", "[DEBUG_RPI] CheckResizeSwapChain()\n");
    // ...
    if (renderSize != currentDimensions)
    {
        AZ_Printf("WindowContext", "[DEBUG_RPI] SwapChain resizing to W:%d H:%d\n", ...);
        // ...
    }
}
```

### `Gems/Atom/Tools/AtomToolsFramework/Code/Source/Viewport/RenderViewportWidget.cpp`

**Función `event()`**: Eliminado el handler de `SurfaceAboutToBeDestroyed` que llamaba a `SendWindowCloseEvent()`.

```cpp
// ANTES (causaba el freeze):
case QEvent::PlatformSurface:
{
    QPlatformSurfaceEvent* surfaceEvent = ...;
    if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
    {
        SendWindowCloseEvent(); // destruía el contexto Vulkan permanentemente
    }
}

// DESPUÉS (correcto):
case QEvent::PlatformSurface:
{
    // Qt emite SurfaceAboutToBeDestroyed transitoriamente durante reparenting de docks.
    // No destruir el contexto: el swapchain se reconstruye por el path normal de resize.
    break;
}
```

## Secuencia de Eventos Corregida

Al arrastrar un dock, la secuencia correcta ahora es:

```
1. Qt reparenta el widget → SurfaceAboutToBeDestroyed (ignorado)
2. vkAcquireNextImageKHR → VK_SUBOPTIMAL_KHR
3. AcquireNewImage normaliza a VK_SUCCESS + programa pendingRecreation
4. Frame se renderiza normalmente
5. ProcessRecreation recrea el swapchain con dimensiones viejas
6. OnWindowResized llega con nuevas dimensiones
7. ResizeInternal recrea el swapchain con dimensiones correctas
8. Render continúa normalmente
```

## Plataforma Afectada

- **Linux + Vulkan + XCB** (window manager con dock flotante)
- El trait `AZ_TRAIT_ATOM_VULKAN_RECREATE_SWAPCHAIN_WHEN_SUBOPTIMAL` está habilitado en Linux (`= 1`)

## Referencias

- Vulkan spec: [vkAcquireNextImageKHR](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkAcquireNextImageKHR.html)
- Qt docs: [QPlatformSurfaceEvent](https://doc.qt.io/qt-6/qplatformsurfaceevent.html)
- O3DE issue tracking: ATOM-4840 (workaround 0x0 swapchain en minimize)
