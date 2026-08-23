# Plan: Migración segura de AZSL → HLSL puro + .srgi → Slang

Objetivo: dejar la sintaxis de autoría de Amazon (AZSL) en favor de **HLSL estándar** para los shaders del fork, con el SRG encapsulado en `.srgi`, y luego migrar el pipeline a **Slang** de forma incremental y sin regresión visual.

Regla global: **sin comentarios nuevos en el código**. Cada fase es un commit aislado, reversible, con validación visual idéntica antes/después.

---

## 0. Estado verificado hoy

| Hecho | Estado |
|---|---|
| `SsgiCompute/SsgiComposite/Restir*` (6 shaders propios) | Cuerpo **ya es HLSL puro**; único resto AZSL = bloque `ShaderResourceGroup PassSrg : SRG_PerPass` inline |
| `azslc` (AZSL→HLSL + reflexión) y `dxc` (HLSL→SPIR-V) | Pipeline actual confirmado en `ShaderBuildArguments.cpp` (campos `azslc`/`dxc`) |
| `slangc` | **No instalado** — binario standalone disponible en releases de Shader-Slang |
| Patrón `.srgi` | Ya existente (`viewsrg_all.srgi`); un SRG completo puede definirse en un `.srgi` e incluirse |
| SRG = contrato de bindeo | Si cambiás nombres/tipos de miembros, el runtime no bindea → mantener SIEMPRE |

Por qué HLSL es el puente a Slang: **Slang es un superset de HLSL** — el cuerpo migrado lo compila `slangc` casi sin cambios; solo la capa SRG necesita un adaptador.

---

## FASE 1 — Migración a HLSL puro + `.srgi`

### 1.0 Formalizar el patrón en los 6 shaders propios (hito fundacional)

1. Crear `plugins/Atom/Feature/Common/Assets/ShaderLib/Atom/Features/PostProcessing/SsgiSrg.srgi` con la declaración COMPLETA del SRG actual (el bloque `ShaderResourceGroup PassSrg : SRG_PerPass` de `SsgiCompute.azsl`, incluido el struct `SsgiConstants` y el Sampler).
2. Igual para ReSTIR: `Assets/ShaderLib/Atom/Features/LightCulling/RestirSrg.srgi` con el SRG de `RestirInitial` (los 4 shaders ReSTIR comparten el patrón; cada uno incluye el que necesita).
3. En cada `.azsl` propio: **borrar el bloque SRG inline** y reemplazarlo por `#include <Atom/Features/...ruta.../XxxSrg.srgi>`.
4. Registrar los `.srgi` nuevos en `Assets/atom_feature_common_asset_files.cmake`.
5. Validar: Asset Processor recompila sin errores; el `diff` de los productos (`restirinitial_vulkan.azshadervariant`, `ssgicompute_vulkan.azshadervariant`) contra el estado pre-cambio es **idéntico** (o sin cambios funcionales); editor se ve igual.
6. Commit: `Fase 1.0 - srgi para SSGI/ReSTIR`.
7. Regla 0: NO tocar nombres ni tipos de miembros del SRG en el `.srgi` — copiar el bloque exacto.

### 1.1 Migrar pases de post-proceso del motor (Tier A)

Candidatos (SRG chico, sin `option bool`, sin red de include de materiales):
- `SsaoCompute.azsl` (PostProcessing)
- `DeferredFog.azsl`, `MobileDeferredFog.azsl`
- `DepthOfFieldMask.azsl`, `DepthOfFieldBokehBlur.azsl`, `DepthOfFieldComposite.azsl`
- `SMAA*` (edge detection, blending weight, neighborhood)
- `CasPass.azsl`
- blurs horizontales/verticales de SSR (`ReflectionScreenSpaceBlur*`)

Procedimiento por shader:
1. `git diff` limpio del archivo (solo el shader a migrar).
2. Copiar el bloque SRG a un `.srgi` nuevo (ruta `Assets/ShaderLib/Atom/Features/<Categoria>/XxxSrg.srgi`), registrar en el cmake de assets.
3. En el `.azsl`: reemplazar el bloque SRG por el include.
4. **No** tocar el cuerpo (aunque sea HLSL) en esta fase — solo la extracción del SRG.
5. Compilar con el AP; amplios de Tabla de validación (sección 4).
6. Si algo cambia visualmente: revertir el archivo (git checkout), recompilar AP, confirmar vuelta al normal; investigar antes de reintentar.
7. Commit por shader (o por grupo de 2-3 del mismo sub-sistema).

### 1.2 Tier B — pases con estado/helpers (cuidado, solo si se necesita)

- `LightCulling.azsl`, `LightCullingRemap.azsl`, `LightCullingTilePrepare.azsl`
- `Taa.azsl` (history/motion), `VolumetricLight*`
- Pasos de sombras proyectadas/ESM

Reglas extra para Tier B:
- Mantener SIEMPRE los `option bool o_*` (si los hay) dentro del `.srgi` — el `azshadervariantlist` depende de ellas.
- No refactorizar `#include` de las `Lights.azsli`/helpers — esa capa queda como está.
- Son candidatos solo si necesitás tocarlos; el beneficio de migrarlos solos es bajo.

### 1.3 Tier C — PBR/materiales (NO migrar salvo necesidad explícita)

- `StandardSurface`, `Lights.azsli`, `ForwardPass*`, `DeferredPassEvaluateLighting`
- Razones: red de includes con macros de herencia (`#ifndef X #define X`), options varias, alto riesgo de regresión del render core sin beneficio (ya son funcionalmente HLSL).
- Si algún día se migran, hacerlo LAST y shader por shader.
---

## FASE 2 — Migración del pipeline a Slang

### 2.0 PoC (prueba de concepto, sin tocar el motor)

1. Descargar `slangc` standalone: releases de https://github.com/shader-slang/slang (binario linux en el release más reciente estable; requiere GLIBC compatible con el sistema).
2. Tomar un shader Ya migrado (p.ej. `SsgiCompute` con su `.srgi`) y compilar directo:
   ```
   slangc SsgiCompute_body.hlsl -target spirv -profile cs_6_0 -entry MainCS -o /tmp/ssgi.spv
   ```
   donde `Ssgi_body.hlsl` es el cuerpo HLSL SIN el bloque SRG (la parte que Slang entiende).
3. Verificar con la reflexión de Slang:
   ```
   slangc SsgiCompute_body.hlsl -target spirv -entry MainCS -reflect-json -o /tmp/ssgi.spv
   ```
   → comparar los bindings (textura/constante/UAV + registros) contra los `.srg.json` actuales del shader.
4. Criterio de salida de la PoC: `spirv-val`/carga Vulkan acepta el SPIR-V, y el JSON de reflexión lista los mismos recursos (en algún orden) que el `srg.json` de Atom. Se documenta en el plan como resultado.
5. Si la PoC falla por bindings/registros: revisar el mapeo (2.2) antes de seguir.

### 2.1 Adaptador del builder (la parte frágil; hacer con máximo cuidado)

Extensión en `plugins/Atom/RHI/Code/Source/RHI.Edit/ShaderBuildArguments.cpp/.h` (punto verificado):
- Añadir `m_slangcPath` y `m_slangcArguments` al struct, siguiendo el patrón de `m_azslcArguments`/`m_dxcArguments` (reflexión de properties ya existente con `azslc`/`dxc`).
- Añadir al `.shader` (descriptor JSON) un campo opcional `"Compiler": "slang"` para marcar shaders migrados.

Flujo nuevo para shader marcado `slang` (en el `ShaderBuilder`/`ShaderPlatformInterface`):
1. Preprocesar includes del `.srgi` → un `.hlsl` con el cuerpo (reutilizar la preparación ya existente del `ShaderAssetBuilder`, sin azslc).
2. `slangc` → `-target spirv` + `-reflect-json`.
3. Convertidor reflexión→JSON de Atom (formato `srg.json`/`bindingdep.json`/`om.json`) respetando los nombres existentes del SRG.
4. Mantener azslc para TODO lo no marcado → **migración híbrida durante la transición** (los shaders Amazon siguen; los tuyos/migrados usan Slang).

Reglas del adaptador:
- El convertidor es la única pieza nueva de código C++ (se escribe tras la PoC con el formato JSON real en mano).
- No cambiar el runtime (RPI/RHI) en esta fase — solo el builder.
- El convertidor debe generar los mismos JSON que azslc para el mismo shader (diff verificable).

### 2.2 Mapeo SRG ↔ Slang

- Cada miembro del SRG (textura, UAV, buffer, constante) → bindig de Slang (register/space) con el **mismo nombre**.
- El convertidor genera el JSON de Atom con esos nombres → el runtime bindea igual (por nombre) sin cambios.
- `option bool` → specialization de Slang (slangc `-D` o specialization constants) — mapear en el convertidor; verificar contra `azshadervariantlist`.
- `ShaderVariantFallback` (SRG designado) → en Slang es la specialization; el convertidor lo traduce al campo del JSON que ya usa Atom.

### 2.3 Migración por módulos (misma disciplina que Fase 1)

- Orden: primero los shaders 1.0/1.1 ya migrados (Ssgi/Restir + post-procesos) marcados `"Compiler": "slang"`.
- Luego, si la validación es sólida, migrar Tier B; Tier C solo a pedido.
- Cada módulo: mismo checklist de validación, commit, rollback.

### 2.4 Aprovechar Slang (post-migración, opcional)

- Features de autoría: modules, generics, interfaces.
- Entrada GLSL literal: `slangc` acepta GLSL (`-target spirv` + sintaxis GLSL) → cubre el objetivo de "lenguaje estándar".
- Autodiff para shader-driven content, etc. — solo para shaders nuevos.
---

## 3. Orden de trabajo recomendado (con hitos verificables)

1. **Fase 1.0** — `.srgi` para Ssgi/Restir (1 sesión). Valida el patrón completo de la fase 1 en terreno propio.
2. **Fase 1.1** — Tier A (SSAO → fog → DOF → blurs), un shader por commit.
3. **Fase 2.0 PoC** — slangc fuera del motor, documentar resultado.
4. **Fase 2.1** — adaptador builder (solo tras PoC OK), con un solo shader de prueba (`SsgiCompute` como piloto).
5. **Fase 2.2/2.3** — mapeo + migración marcada `"Compiler": "slang"` por módulo.
6. (Opcional) 2.4 features de Slang en shaders nuevos.

Cada hito termina con: AP sin errores + imagen idéntica + commit.

---

## 4. Checklist de validación (por shader/fase)

1. `git diff` del shader: solo el bloque SRG extraído (Fase 1) o solo el `"Compiler": "slang"` + cuerpo (Fase 2). Nada más.
2. Asset Processor: `0 errors, 0 warnings` en el joblog del shader (filtro: `S: 0 errors`).
3. Productos generados: `.azshader` + `.azshadervariant` presentes para las 4 plataformas (vulkan incluida).
4. **Diff de productos**: comparar los `.srg.json`/`.bindingdep.json` del shader contra el commit anterior — si son idénticos, la reflexión está intacta.
5. **Visual**: abrir escena representativa (SponzaRT), comparar captura antes/después (misma cámara/luz): píxeles indistinguibles en las zonas del pass migrado.
6. CP (empírico): `r_gpuFrame` antes/después — sin delta anómalo.
7. Log del editor: sin warnings nuevos de bindeo ("could not bind shader buffer index ... because it has no attachment").

Cualquier fallo en 4/5/7 → **rollback del shader** (git checkout), recompilar AP, confirmar vuelta al estado normal antes de seguir.

---

## 5. Riesgos y mitigaciones

| Riesgo | Mitigación |
|---|---|
| Cambiar un nombre/tipo del SRG rompe el bindeo en runtime | Regla 0: copiar el bloque exacto al `.srgi`; diff de productos (checklist 4) detecta |
| Slang no reproduce la reflexión de Atom (registros/espacios) | PoC 2.0 documenta el mapeo antes de tocar el builder |
| `option bool` / variantes no migran 1:1 a Slang | Mantenerlas en `.srgi` en Fase 1; mapeo a specialization en Fase 2 verificando el variantlist |
| `ShaderVariantFallback` (SRG designado) mal traducido | Hacer que el convertidor lo genere explícitamente; validar con un shader que tenga options |
| Dependencia de binario nuevo (slangc) en el build | Versionar el binario en `3p` (o descargarlo con checksum fijo) SOLO cuando 2.1 pase |
| Regresión visual no detectada | Checklist 5 (captura SponzaRT antes/después) por cada commit |
| Fase 1.3 (PBR) muy riesgosa | No migrar salvo pedido explícito; si se hace: último, shader por shader |

---

## 6. Fuera de alcance (por ahora)

- Migrar el runtime RPI/RHI a Slang (solo el builder).
- Migrar material PBR/macros de herencia de Amazon.
- Usar features avanzadas de Slang antes de que 2.1 pase.
- Tocar los `.azsl` del engine que no van a migrar (Tier C).

---

## 7. Recursos

- Binario `slangc`: https://github.com/shader-slang/slang/releases (standalone linux).
- Reflexión de Slang: `slangc -reflect-json` (formato documentado en la wiki del proyecto).
- Formato de productos de Atom (`.srg.json`, `.bindingdep.json`): `plugins/Atom/RHI/Code/Include/Atom/RHI.Reflect/Shader/` (estructuras `ShaderResourceGroupLayout`, `BindingDependencies`).
