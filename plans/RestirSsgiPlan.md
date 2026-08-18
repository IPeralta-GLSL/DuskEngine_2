# Plan de implementación: SSGI + ReSTIR Light Sampling para DuskEngine

Base: técnicas porteadas desde `Expiration/Capsaicin` (AMD GPUOpen) al renderer Atom de este fork.
Regla global: **sin comentarios en el código**. Todo el código nuevo usa los patrones ya verificados del fork (SSAO/CACAO como modelo de pass, LightCulling como modelo de binding de luces).

---

## 0. Resumen ejecutivo

| Técnica | Origen Capsaicin | Destino Atom | Fases | Esfuerzo |
|---|---|---|---|---|
| **SSGI** (horizon-based GI + AO) | `render_techniques/ssgi/ssgi.comp` | ComputePass + composite aditivo, colgado de `OpaqueParent` después de SSAO | A (única) | 2-3 días |
| **ReSTIR DI** (reservoir light sampling) | `lights/reservoir.hlsl` | 2 ComputePass (initial + spatial) + integración en shading deferred | B0 (reservoirs + debug), B1 (integración) | 1-2 semanas |

Arquitectura elegida: **todo dentro de `plugins/Atom/Feature/Common`** (mismo lugar que SSAO y LightCulling), sin gems nuevos. Los shaders se escriben en AZSL y se compilan por el pipeline existente (HLSL→SPIR-V con DXC).

## 0.1 Estado actual verificado en el repo (actualización)
La mayoría de los archivos de este plan **ya existen** en el fork. Estado real:
| Pieza | Estado |
|---|---|
| SSGI: `SsgiPasses.{h,cpp}`, `SsgiParent/Compute/Composite.pass`, `SsgiCompute/Composite.{azsl,shader}` | ✅ Creados en `plugins/Atom/Feature/Common/` |
| SSGI: registro C++ (`CommonSystemComponent.cpp:291-296`), cmake (`atom_feature_common_files.cmake:272-273`) | ✅ Hecho |
| SSGI: PassRequest "Ssgi" en `OpaqueParent.pass:621-639` y `RTShadowCompositePass` consumiendo `Ssgi.Output` (líneas 645-651) | ✅ Hecho |
| SSGI: `SsgiCompute.azsl` usa los nombres reales del ViewSrg (`m_viewMatrixInverse`, `m_viewProjectionInverseMatrix`, `m_worldPosition`) | ✅ Corregido respecto al borrador de A.2.1 |
| ReSTIR: `RestirReservoir.azsli`, `RestirInitial/RestirSpatial.{azsl,shader,shadervariantlist}`, `RestirParent/Initial/Spatial.pass`, `RestirPasses.{h,cpp}` | ✅ Esqueleto creado en `plugins/Atom/Feature/Common/` |
| ReSTIR: registro C++ (`CommonSystemComponent.cpp:299-300`), cmake (`atom_feature_common_files.cmake:55-56`) | ✅ Hecho |
| ReSTIR: PassRequest "Restir" en `OpaqueParent.pass:574-599` | ⚠️ Existe pero su `Output` **no se consume** en ningún sitio |
| Templates `Ssgi*` y `Restir*` en `Assets/Passes/PassTemplates.azasset` | ❌ **Faltan** — sin esto la carga del pipeline falla |
| Assets nuevos en `Assets/atom_feature_common_asset_files.cmake` | ❌ **Faltan** (0 coincidencias de `Restir`/`Ssgi`) |
| ReSTIR: merge espacial con re-evaluación del target pdf (shift mapping) | ❌ El `RestirSpatial.azsl` actual usa `candidateTargetPdf = 1.0` (sesgado) |
| ReSTIR: muestreo uniforme de candidatos del tile | ⚠️ Toma los primeros 8 del tile (sesgo por orden de lista) |
| ReSTIR: pass de visualización debug (B0) | ❌ No existe |
| ReSTIR: integración con el shading (B1) | ❌ No existe |
| Build + verificación en Editor | ❌ Pendiente |
**Consecuencia**: la Parte A se reduce a A.5 (registro de templates + assets + build). La Parte B pasa a ser el cuerpo principal del trabajo, detallada abajo sobre el esqueleto existente.

---

## 1. Base técnica verificada en el fork (puntos de anclaje)

1. **Patrón de pass screen-space**: `SsaoParentPass` (ParentPass) + `SsaoComputePass` (ComputePass con `ComputePassData` + `FullscreenDispatch`) + `ModulateTextureTemplate` como composite. Archivos: `Code/Source/PostProcessing/SsaoPasses.{h,cpp}`, `Assets/Passes/SsaoParent.pass`, `Assets/Passes/SsaoCompute.pass`, `Assets/Shaders/PostProcessing/SsaoCompute.{azsl,shader}`.
2. **Hookup del pipeline**: `Assets/Passes/OpaqueParent.pass` contiene el PassRequest "Ssao" (~línea 575) con slots `Modulate` y `DepthLinear`; el consumidor siguiente (`RTShadowCompositePass`) referencia el output de "Ssao".
3. **Luces**: los buffers viven en **ViewSrg** (`ViewSrg::m_simplePointLights[lightIndex]`, etc.). El culling produce `m_tileLightData` (Texture2D<uint4>) + `m_lightListRemapped` (StructuredBuffer<uint>) consumidos vía `LightCullingTileIterator` (`Assets/ShaderLib/Atom/Features/LightCulling/LightCullingTileIterator.azsli`). Evaluación por tipo: `SimplePointLightUtil::Init(srgLight, surface)` + `light.Apply(...)` en `ForwardPass*Lights.azsli`.
4. **Registro de passes**: `Code/Source/CommonSystemComponent.cpp` → `passSystem->AddPassCreator(...)`.
5. **CMake**: los fuentes se listan en el `.cmake` que contiene `Source/PostProcessing/SsaoPasses.cpp` (verificar cuál de `Code/atom_feature_common_*.cmake` lo lista al implementar; agregar en el mismo).

---

# PARTE A — SSGI

## A.1 Arquitectura

```
OpaqueParent
  └── Ssao (existente, modula AO)
        └── Ssgi (SsgiParentTemplate)                 [nuevo]
              ├── SsgiCompute  (compute 8x8, fullscreen dispatch)
              │     in : DepthLinear, Lighting (output de Ssao)
              │     out: Ssgi RGBA16F (rgb = GI, a = AO propia)
              └── SsgiComposite (compute, fullscreen dispatch)
                    in : Ssgi texture
                    io : Lighting (output de Ssao)  →  *= AO, += GI*intensity
RTShadowCompositePass pasa a referenciar el output de "Ssgi" (antes "Ssao")
```

Puerto del SSGI de Capsaicin (horizon-based, de h3r2tic): integra arcos de horizonte por slice en 2 direcciones, acumulando lighting de vecinos ponderado por el arco (GI near-field) y el occlusion integral (AO). Diferencias: normal derivada del depth (no requiere attachment de normales), ruido IGN en vez de blue noise, salida en una sola textura RGBA16F.

## A.2 Archivos nuevos

### A.2.1 `plugins/Atom/Feature/Common/Assets/Shaders/PostProcessing/SsgiCompute.azsl`

```hlsl
#include <Atom/Features/SrgSemantics.azsli>

#include <scenesrg_all.srgi>
#include <viewsrg_all.srgi>

#include <Atom/RPI/Math.azsli>

struct SsgiConstants
{
    float m_uvRadius;
    float m_falloffMul;
    float m_falloffAdd;
    float m_intensity;
    float m_aoStrength;
    uint m_sliceCount;
    uint m_stepCount;
    uint m_frameIndex;
    float m_padding;
};

ShaderResourceGroup PassSrg : SRG_PerPass
{
    Texture2D<float> m_depthLinear;
    Texture2D<float4> m_lighting;
    RWTexture2D<float4> m_ssgiOutput;
    SsgiConstants m_constants;

    Sampler PointSampler
    {
        MinFilter = Point;
        MagFilter = Point;
        MipFilter = Point;
        AddressU = Clamp;
        AddressV = Clamp;
        AddressW = Clamp;
    };
}

static const float SSGI_PI = 3.14159265359;
static const float SSGI_HALF_PI = 1.57079632679;

float SsgiLuminance(float3 c)
{
    return dot(c, float3(0.2126, 0.7152, 0.0722));
}

float SsgiIgn(uint2 pixel, uint frame)
{
    float2 p = float2(pixel) + float2(5.392753, 5.287193) * frame;
    return frac(52.9829189 * frac(dot(p, float2(0.06711056, 0.00583715))));
}

float3 SsgiCameraForward()
{
    return normalize(mul(ViewSrg::ViewToWorld, float4(0.0, 0.0, 1.0, 0.0)).xyz);
}

float3 SsgiWorldPosition(float2 uv, float linearDepth, float3 camPos, float3 camFwd)
{
    float4 ndc = float4(uv * 2.0 - 1.0, 1.0, 1.0);
    float3 farPoint = mul(ViewSrg::ClipToWorld, ndc).xyz;
    float3 rayDir = normalize(farPoint - camPos);
    float t = linearDepth / max(dot(rayDir, camFwd), 1e-6);
    return camPos + rayDir * t;
}

float SsgiIntersectDirPlaneOneSided(float3 dir, float3 normal, float3 pt)
{
    float d = -dot(pt, normal);
    float t = d / max(1e-5, -dot(dir, normal));
    return t;
}

float SsgiIntegrateHalfArc(float horizonAngle, float normalAngle)
{
    return (cos(normalAngle) + 2.0 * horizonAngle * sin(normalAngle) - cos(2.0 * horizonAngle - normalAngle)) / 4.0;
}

float3 SsgiComputeNormal(float2 uv, float depth, uint2 dims, float3 camPos, float3 camFwd)
{
    float2 texel = 1.0 / float2(dims);
    float dx = PassSrg::m_depthLinear.SampleLevel(PassSrg::PointSampler, min(uv + float2(texel.x, 0), 1.0), 0);
    float dy = PassSrg::m_depthLinear.SampleLevel(PassSrg::PointSampler, min(uv + float2(0, texel.y), 1.0), 0);
    float3 p = SsgiWorldPosition(uv, depth, camPos, camFwd);
    float3 px = SsgiWorldPosition(uv + float2(texel.x, 0), dx, camPos, camFwd);
    float3 py = SsgiWorldPosition(uv + float2(0, texel.y), dy, camPos, camFwd);
    float3 n = normalize(cross(px - p, py - p));
    return dot(n, camFwd) < 0.0 ? -n : n;
}

[numthreads(8, 8, 1)]
void MainCS(uint3 dispatchId : SV_DispatchThreadID)
{
    uint2 dims;
    PassSrg::m_ssgiOutput.GetDimensions(dims.x, dims.y);
    if (any(dispatchId.xy >= dims))
    {
        return;
    }

    float2 uv = (dispatchId.xy + 0.5) / float2(dims);
    float depth = PassSrg::m_depthLinear.SampleLevel(PassSrg::PointSampler, uv, 0);
    if (depth <= 0.0)
    {
        PassSrg::m_ssgiOutput[dispatchId.xy] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float3 camPos = ViewSrg::CameraPosition;
    float3 camFwd = SsgiCameraForward();
    float3 worldPos = SsgiWorldPosition(uv, depth, camPos, camFwd);
    float3 worldNormal = SsgiComputeNormal(uv, depth, dims, camPos, camFwd);
    float3 eyeDir = normalize(camPos - worldPos);

    float noise = SsgiIgn(dispatchId.xy, PassSrg::m_constants.m_frameIndex);

    float sliceUvRadius = PassSrg::m_constants.m_uvRadius / max(depth, 1e-4);
    float ambientOcclusion = 0.0;
    float3 globalLighting = float3(0.0, 0.0, 0.0);

    [loop]
    for (uint sliceIndex = 0; sliceIndex < PassSrg::m_constants.m_sliceCount; ++sliceIndex)
    {
        float sliceAngle = ((sliceIndex + noise) / PassSrg::m_constants.m_sliceCount) * SSGI_PI;
        float sliceCos = cos(sliceAngle);
        float sliceSin = sin(sliceAngle);
        float2 sliceUvDir = float2(sliceCos, -sliceSin) * sliceUvRadius;

        float3 sliceViewDir = float3(sliceCos, sliceSin, 0.0);
        float3 sliceWorldDir = mul(ViewSrg::ViewToWorld, float4(sliceViewDir, 0.0)).xyz;
        float3 orthoWorldDir = sliceWorldDir - dot(sliceWorldDir, eyeDir) * eyeDir;
        float3 projAxisDir = normalize(cross(orthoWorldDir, eyeDir));
        float3 projWorldNormal = worldNormal - dot(worldNormal, projAxisDir) * projAxisDir;
        float projWorldNormalLen = length(projWorldNormal);
        float projWorldNormalCos = saturate(dot(projWorldNormal, eyeDir) / max(projWorldNormalLen, 1e-6));
        float projWorldNormalAngle = sign(dot(orthoWorldDir, projWorldNormal)) * acos(projWorldNormalCos);

        float sideSigns[2] = { 1.0, -1.0 };
        float horizonAngles[2];
        [unroll]
        for (int sideIndex = 0; sideIndex < 2; ++sideIndex)
        {
            float horizonMin = cos(projWorldNormalAngle + sideSigns[sideIndex] * SSGI_HALF_PI);
            float horizonCos = horizonMin;
            float3 prevSampleWorldPos = float3(0.0, 0.0, 0.0);

            [loop]
            for (uint stepIndex = 0; stepIndex < PassSrg::m_constants.m_stepCount; ++stepIndex)
            {
                float sampleStep = ((stepIndex + noise) / PassSrg::m_constants.m_stepCount);
                float2 sampleUv = uv + sideSigns[sideIndex] * sampleStep * sliceUvDir;
                sampleUv = clamp(sampleUv, float2(0.0, 0.0), float2(1.0, 1.0));
                float sampleDepth = PassSrg::m_depthLinear.SampleLevel(PassSrg::PointSampler, sampleUv, 0);
                if (sampleDepth <= 0.0)
                {
                    prevSampleWorldPos = worldPos;
                    continue;
                }
                float3 sampleWorldPos = SsgiWorldPosition(sampleUv, sampleDepth, camPos, camFwd);

                float3 horizonWorldDir = sampleWorldPos - worldPos;
                float horizonWorldLen = length(horizonWorldDir);
                horizonWorldDir /= max(horizonWorldLen, 1e-6);
                float sampleWeight = saturate(horizonWorldLen * PassSrg::m_constants.m_falloffMul + PassSrg::m_constants.m_falloffAdd);
                float sampleCos = lerp(horizonMin, dot(horizonWorldDir, eyeDir), sampleWeight);
                float prevHorizonCos = horizonCos;
                horizonCos = max(horizonCos, sampleCos);

                [branch]
                if (sampleCos >= prevHorizonCos && horizonWorldLen > 1e-3)
                {
                    float3 sampleLighting = PassSrg::m_lighting.SampleLevel(PassSrg::PointSampler, sampleUv, 0).rgb;
                    float3 sampleWorldNormal = SsgiComputeNormal(sampleUv, sampleDepth, dims, camPos, camFwd);

                    float3 closestWorldPos = prevSampleWorldPos;
                    if (stepIndex > 0)
                    {
                        float3 prevScaled = prevSampleWorldPos * min(
                            SsgiIntersectDirPlaneOneSided(prevSampleWorldPos, sampleWorldNormal, sampleWorldPos),
                            SsgiIntersectDirPlaneOneSided(prevSampleWorldPos, worldNormal, worldPos));
                        if (SsgiLuminance(abs(prevScaled)) > 1e-9)
                        {
                            closestWorldPos = prevScaled;
                        }
                    }

                    float horizonAngle0 = projWorldNormalAngle + max(sideSigns[sideIndex] * acos(clamp(prevHorizonCos, -1.0, 1.0)) - projWorldNormalAngle, -SSGI_HALF_PI);
                    float horizonAngle1 = projWorldNormalAngle + min(sideSigns[sideIndex] * acos(clamp(horizonCos, -1.0, 1.0)) - projWorldNormalAngle, SSGI_HALF_PI);
                    float sampleOcclusion = SsgiIntegrateHalfArc(horizonAngle0, projWorldNormalAngle) - SsgiIntegrateHalfArc(horizonAngle1, projWorldNormalAngle);

                    float backface = step(0.0, dot(-horizonWorldDir, sampleWorldNormal));
                    globalLighting += sampleLighting * sampleOcclusion * backface;
                }

                prevSampleWorldPos = sampleWorldPos;
            }

            horizonAngles[sideIndex] = sideSigns[sideIndex] * acos(clamp(horizonCos, -1.0, 1.0));
            ambientOcclusion += projWorldNormalLen * SsgiIntegrateHalfArc(horizonAngles[sideIndex], projWorldNormalAngle);
            globalLighting *= projWorldNormalLen;
        }
    }

    ambientOcclusion = saturate(ambientOcclusion / PassSrg::m_constants.m_sliceCount);
    globalLighting = globalLighting / PassSrg::m_constants.m_sliceCount * PassSrg::m_constants.m_intensity;

    PassSrg::m_ssgiOutput[dispatchId.xy] = float4(globalLighting, ambientOcclusion);
}
```

### A.2.2 `SsgiCompute.shader`

```json
{
    "Source": "SsgiCompute.azsl",
    "ProgramSettings" :
    {
        "EntryPoints":
        [
        {
            "name" : "MainCS",
            "type" : "Compute"
        }
        ]
    }
}
```

### A.2.3 `SsgiCompute.shadervariantlist`

```json
{
    "unshadedVariants":
    [
        { "options": [], "hash": "autogen", "stages": [] }
    ],
    "shadedVariants":
    [
        { "options": [], "hash": "autogen", "stages": [] }
    ]
}
```

### A.2.4 `plugins/Atom/Feature/Common/Assets/Shaders/PostProcessing/SsgiComposite.azsl`

```hlsl
#include <Atom/Features/SrgSemantics.azsli>

ShaderResourceGroup PassSrg : SRG_PerPass
{
    Texture2D<float4> m_ssgi;
    RWTexture2D<float4> m_inputOutput;
    float m_aoStrength;

    Sampler LinearSampler
    {
        MinFilter = Linear;
        MagFilter = Linear;
        MipFilter = Linear;
        AddressU = Clamp;
        AddressV = Clamp;
        AddressW = Clamp;
    };
}

[numthreads(16, 16, 1)]
void MainCS(uint3 dispatchId : SV_DispatchThreadID)
{
    uint2 dims;
    PassSrg::m_inputOutput.GetDimensions(dims.x, dims.y);
    if (any(dispatchId.xy >= dims))
    {
        return;
    }

    float2 pixelSize = 1.0 / float2(dims);
    float2 uv = (dispatchId.xy + 0.5) * pixelSize;

    float4 ssgi = PassSrg::m_ssgi.SampleLevel(PassSrg::LinearSampler, uv, 0);

    float3 lighting = PassSrg::m_inputOutput[dispatchId.xy].rgb;
    float ao = lerp(1.0, 1.0 - ssgi.a, saturate(PassSrg::m_aoStrength));
    float3 result = lighting * ao + ssgi.rgb;

    PassSrg::m_inputOutput[dispatchId.xy] = float4(result, 1.0);
}
```

### A.2.5 `SsgiComposite.shader`

Mismo formato que A.2.2 con `MainCS` y `SsgiComposite.azsl`.

### A.2.6 `plugins/Atom/Feature/Common/Code/Source/PostProcessing/SsgiPasses.h`

```cpp
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ
{
    namespace Render
    {
        class SsgiParentPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(SsgiParentPass);

        public:
            AZ_RTTI(AZ::Render::SsgiParentPass, "{7C4F1A22-9E53-4B71-8A0D-2F5E93C101AA}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(SsgiParentPass, SystemAllocator);
            virtual ~SsgiParentPass() = default;

            static RPI::Ptr<SsgiParentPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            void InitializeInternal() override;

        private:
            SsgiParentPass(const RPI::PassDescriptor& descriptor);
        };

        class SsgiComputePass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(SsgiComputePass);

        public:
            AZ_RTTI(AZ::Render::SsgiComputePass, "{3D8B7C44-1A69-4F0E-9C2B-6E71D4A8F3B5}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(SsgiComputePass, SystemAllocator);
            virtual ~SsgiComputePass() = default;

            static RPI::Ptr<SsgiComputePass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            SsgiComputePass(const RPI::PassDescriptor& descriptor);

            struct SsgiConstants
            {
                float m_uvRadius = 0.16f;
                float m_falloffMul = 0.03f;
                float m_falloffAdd = -0.3f;
                float m_intensity = 1.0f;
                float m_aoStrength = 0.0f;
                uint32_t m_sliceCount = 2;
                uint32_t m_stepCount = 8;
                uint32_t m_frameIndex = 0;
                float m_padding = 0.0f;
            };

            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
            uint32_t m_frameIndex = 0;
        };
    }
}
```

### A.2.7 `plugins/Atom/Feature/Common/Code/Source/PostProcessing/SsgiPasses.cpp`

```cpp
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>

#include "SsgiPasses.h"

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<SsgiParentPass> SsgiParentPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<SsgiParentPass> pass = aznew SsgiParentPass(descriptor);
            return pass;
        }

        SsgiParentPass::SsgiParentPass(const RPI::PassDescriptor& descriptor)
            : ParentPass(descriptor)
        {
        }

        void SsgiParentPass::InitializeInternal()
        {
            ParentPass::InitializeInternal();
        }

        RPI::Ptr<SsgiComputePass> SsgiComputePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<SsgiComputePass> pass = aznew SsgiComputePass(descriptor);
            return pass;
        }

        SsgiComputePass::SsgiComputePass(const RPI::PassDescriptor& descriptor)
            : ComputePass(descriptor)
        {
        }

        void SsgiComputePass::FrameBeginInternal(FramePrepareParams params)
        {
            ComputePass::FrameBeginInternal(params);

            m_frameIndex = (m_frameIndex + 1) & 63;

            if (m_shaderResourceGroup != nullptr)
            {
                SsgiConstants constants;
                constants.m_frameIndex = m_frameIndex;
                m_shaderResourceGroup->SetConstant(m_constantsIndex, constants);
            }
        }
    }
}
```

### A.2.8 `plugins/Atom/Feature/Common/Assets/Passes/SsgiCompute.pass`

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "PassAsset",
    "ClassData": {
        "PassTemplate": {
            "Name": "SsgiComputeTemplate",
            "PassClass": "SsgiComputePass",
            "Slots": [
                {
                    "Name": "DepthLinear",
                    "SlotType": "Input",
                    "ScopeAttachmentUsage": "Shader"
                },
                {
                    "Name": "Lighting",
                    "SlotType": "Input",
                    "ScopeAttachmentUsage": "Shader"
                },
                {
                    "Name": "Output",
                    "SlotType": "Output",
                    "ScopeAttachmentUsage": "Shader",
                    "LoadStoreAction": {
                        "LoadAction": "Clear",
                        "StoreAction": "Store"
                    }
                }
            ],
            "ImageAttachments": [
                {
                    "Name": "SsgiTexture",
                    "SizeSource": {
                        "Source": {
                            "Pass": "This",
                            "Attachment": "DepthLinear"
                        }
                    },
                    "ImageDescriptor": {
                        "Format": "RGBA16_FLOAT"
                    }
                }
            ],
            "Connections": [
                {
                    "LocalSlot": "Output",
                    "AttachmentRef": {
                        "Pass": "This",
                        "Attachment": "SsgiTexture"
                    }
                }
            ],
            "PassData": {
                "$type": "ComputePassData",
                "ShaderAsset": {
                    "FilePath": "Shaders/PostProcessing/SsgiCompute.shader"
                },
                "FullscreenDispatch": true
            }
        }
    }
}
```

### A.2.9 `plugins/Atom/Feature/Common/Assets/Passes/SsgiComposite.pass`

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "PassAsset",
    "ClassData": {
        "PassTemplate": {
            "Name": "SsgiCompositeTemplate",
            "PassClass": "ComputePass",
            "Slots": [
                {
                    "Name": "Ssgi",
                    "SlotType": "Input",
                    "ScopeAttachmentUsage": "Shader"
                },
                {
                    "Name": "InputOutput",
                    "SlotType": "InputOutput",
                    "ScopeAttachmentUsage": "Shader"
                }
            ],
            "PassData": {
                "$type": "ComputePassData",
                "ShaderAsset": {
                    "FilePath": "Shaders/PostProcessing/SsgiComposite.shader"
                },
                "FullscreenDispatch": true
            }
        }
    }
}
```

### A.2.10 `plugins/Atom/Feature/Common/Assets/Passes/SsgiParent.pass`

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "PassAsset",
    "ClassData": {
        "PassTemplate": {
            "Name": "SsgiParentTemplate",
            "PassClass": "SsgiParentPass",
            "Slots": [
                {
                    "Name": "Modulate",
                    "SlotType": "Input",
                    "ScopeAttachmentUsage": "Shader"
                },
                {
                    "Name": "DepthLinear",
                    "SlotType": "Input",
                    "ScopeAttachmentUsage": "Shader"
                },
                {
                    "Name": "Output",
                    "SlotType": "Output",
                    "ScopeAttachmentUsage": "Shader"
                }
            ],
            "Connections": [
                {
                    "LocalSlot": "Output",
                    "AttachmentRef": {
                        "Pass": "SsgiComposite",
                        "Attachment": "InputOutput"
                    }
                }
            ],
            "PassRequests": [
                {
                    "Name": "SsgiCompute",
                    "TemplateName": "SsgiComputeTemplate",
                    "Enabled": true,
                    "Connections": [
                        {
                            "LocalSlot": "DepthLinear",
                            "AttachmentRef": {
                                "Pass": "Parent",
                                "Attachment": "DepthLinear"
                            }
                        },
                        {
                            "LocalSlot": "Lighting",
                            "AttachmentRef": {
                                "Pass": "Parent",
                                "Attachment": "Modulate"
                            }
                        }
                    ]
                },
                {
                    "Name": "SsgiComposite",
                    "TemplateName": "SsgiCompositeTemplate",
                    "Enabled": true,
                    "Connections": [
                        {
                            "LocalSlot": "Ssgi",
                            "AttachmentRef": {
                                "Pass": "SsgiCompute",
                                "Attachment": "Output"
                            }
                        },
                        {
                            "LocalSlot": "InputOutput",
                            "AttachmentRef": {
                                "Pass": "Parent",
                                "Attachment": "Modulate"
                            }
                        }
                    ]
                }
            ]
        }
    }
}
```

## A.3 Modificaciones a archivos existentes

### A.3.1 `Assets/Passes/OpaqueParent.pass`

Insertar el PassRequest de Ssgi inmediatamente después del bloque "Ssao" (~línea 589):

```json
                {
                    "Name": "Ssgi",
                    "TemplateName": "SsgiParentTemplate",
                    "Connections": [
                        {
                            "LocalSlot": "Modulate",
                            "AttachmentRef": {
                                "Pass": "Ssao",
                                "Attachment": "Output"
                            }
                        },
                        {
                            "LocalSlot": "DepthLinear",
                            "AttachmentRef": {
                                "Pass": "Parent",
                                "Attachment": "DepthLinear"
                            }
                        }
                    ]
                },
```

Y actualizar el consumidor siguiente (`RTShadowCompositePass`, conexión `DiffuseInput`) para que referencie `"Pass": "Ssgi"` en vez de `"Pass": "Ssao"`.

### A.3.2 `Code/Source/CommonSystemComponent.cpp`

```cpp
passSystem->AddPassCreator(Name("SsgiParentPass"), &SsgiParentPass::Create);
passSystem->AddPassCreator(Name("SsgiComputePass"), &SsgiComputePass::Create);
```

Agregar junto a los de SSAO (líneas ~288-289) con `#include "PostProcessing/SsgiPasses.h"`.

### A.3.3 CMake

Agregar `Source/PostProcessing/SsgiPasses.h` y `Source/PostProcessing/SsgiPasses.cpp` en el mismo `.cmake` que lista `Source/PostProcessing/SsaoPasses.cpp`.

## A.4 Parámetros por defecto (tuning)

| Constante | Default | Rango útil |
|---|---|---|
| `m_uvRadius` | 0.16 | 0.05-0.35 |
| `m_sliceCount` | 2 | 1-4 |
| `m_stepCount` | 8 | 4-16 |
| `m_falloffMul/Add` | 0.03 / -0.3 | distancia efectiva |
| `m_intensity` | 1.0 | 0-2 |
| `m_aoStrength` | 0.0 | 0-1 (0 porque CACAO ya aplica AO) |

---

## A.5 Trabajo restante de SSGI (cierre de la Parte A)
Todo el código y los passes de SSGI ya están creados. Solo queda:
1. **Registrar los templates** en `plugins/Atom/Feature/Common/Assets/Passes/PassTemplates.azasset`, tras la entrada `SsaoComputeTemplate` (~línea 310), mismo formato:
   ```json
            {
                "Name": "SsgiParentTemplate",
                "Path": "Passes/SsgiParent.pass"
            },
            {
                "Name": "SsgiComputeTemplate",
                "Path": "Passes/SsgiCompute.pass"
            },
            {
                "Name": "SsgiCompositeTemplate",
                "Path": "Passes/SsgiComposite.pass"
            },
   ```
2. **Listar los assets** en `plugins/Atom/Feature/Common/Assets/atom_feature_common_asset_files.cmake`: los 3 `.pass`, los 2 `.shader`, los 2 `.azsl` y el `.shadervariantlist` de SSGI (misma sección donde están los de SSAO).
3. **Build y verificación**: compilar `Atom_Feature_Common` + `Editor`, dejar que el Asset Processor genere los `.azshadervariant`, abrir el Editor y comprobar en el Pass Tool que `SsgiCompute`/`SsgiComposite` aparecen bajo `OpaqueParent` y que la imagen no cambia con `m_intensity = 0` / `m_aoStrength = 0`.
4. **Criterio de aceptación A**: sin errores de carga de templates en el log; el pass graph muestra la cadena `Ssao → Ssgi → RTShadowCompositePass`; activando `m_intensity > 0` se aprecia bounce de color en esquinas/contactos.
> Nota: el borrador de A.2.1 usaba `ViewSrg::ViewToWorld` / `ClipToWorld` / `CameraPosition`; los nombres reales del ViewSrg del fork son `m_viewMatrixInverse`, `m_viewProjectionInverseMatrix` y `m_worldPosition` (`Assets/ShaderResourceGroups/ViewSrg.azsli:17-25`). El `SsgiCompute.azsl` ya creado usa los nombres correctos.

---

# PARTE B — ReSTIR DI (reservoir light sampling)
Puerto de `Expiration/Capsaicin/src/core/src/lights/reservoir.hlsl` al esqueleto ya existente en `plugins/Atom/Feature/Common`. Capsaicin usa sus reservoirs dentro de GI1 con hash grid world-space; aquí se hace **ReSTIR DI clásico en screen-space** (Bitterli et al. 2020: initial candidates + spatial reuse), que es lo que el esqueleto del fork ya apunta.

## B.1 Correspondencia Capsaicin → fork
| Concepto | Capsaicin (`reservoir.hlsl`) | Fork (`RestirReservoir.azsli`) | Acción |
|---|---|---|---|
| Reservoir | `LightSample{index, sampleParams} + M + W + visibility` | `RestirReservoir{m_lightSample, m_M, m_W}` | OK. **Sin `visibility`**: las simple point lights de Atom no tienen sombras (`ForwardPassSimplePointLights.azsli:27`), no hay shadow ray que cachear |
| Packing | `uint4` (index u32, params 2×f16, W f32, M f16 \| vis f16) | `PackRestirReservoir` → `uint4` idéntico (word `.w` usa solo M) | OK, textura `RGBA32_UINT` fullscreen |
| Update RIS | MIS pairwise `M/(M+1)` por muestra (`updateReservoirRadianceBRDF`, l.175-209) | RIS clásico `Σw` (`RestirUpdate` + `RestirFinalize`) | Mantener RIS clásico en B0 (más simple, correcto); MIS pairwise = upgrade opcional |
| Target pdf | `luminance(BRDF_normalizada * radiancia)` (l.97-135) | `RestirEvaluateTargetPdf` = `luminance(rgbIntensity) * NdotL * ventana_atenuación` | OK: la BRDF normalizada de Capsaicin cancela el albedo; el del fork es el equivalente difuso |
| Merge espacial | Re-evalúa la muestra del vecino **en el punto actual** (`mergeReservoirs`, l.373-461) | `RestirSpatial.azsl` pasa `candidateTargetPdf = 1.0` | ❌ **Fix obligatorio B0** (B.3) |
| Clamp temporal | `M ≤ 20` (`Reservoir_ClampPrevious`, l.94) | `RestirClampM(..., 20.0)` | OK |
| Candidatos M | 8 (`kReservoir_SampleCount`) | `RESTIR_CANDIDATE_COUNT 8` | OK |
| Fuente de luces | `g_LightBuffer` plano + `getNumberLights()` | `ViewSrg::m_simplePointLights` + `m_visibleSimplePointLightCount` vía `LightCullingTileIterator` | OK: los índices de `m_lightListRemapped` indexan directo `m_simplePointLights` (patrón de `ForwardPassSimplePointLights.azsli:54-57`) |

## B.2 Arquitectura final
```
OpaqueParent
  └── Restir (RestirParentTemplate)
        ├── RestirInitial   (RIS: M=8 candidatos uniformes del tile)   [existente, fix B.4]
        ├── RestirSpatial   (merge K vecinos con shift mapping)        [existente, fix B.3]
        ├── RestirDebug     (visualización, Enabled=false)             [nuevo, B0 — B.5]
        └── RestirApply     (evalúa la luz ganadora, aditivo)          [nuevo, B1 — B.7]
Cadena de Lighting (B1):  SubsurfaceScattering.Output
    → Restir.Lighting (RestirApply, += contribución)
    → Ssao.Modulate → Ssgi.Modulate → RTShadowCompositePass.DiffuseInput
```
Restricción del esqueleto actual: solo **simple point lights**. Spots/direccionales/área quedan como fases futuras (B.9).

## B.3 Fix obligatorio: shift mapping en `RestirSpatial.azsl`
El merge actual es sesgado: usa `1.0` como target pdf del vecino y multiplica ambos pesos por `similarity` (la `similarity` se cancela en la selección pero corrompe `W`). Cambios:
1. **Mover `RestirEvaluateTargetPdf`** desde `RestirInitial.azsl` a `RestirReservoir.azsli` (va después de `RestirLuminance`; el archivo se incluye siempre tras `viewsrg_all.srgi`, así que `ViewSrg::m_simplePointLights` está disponible). Firmarla igual: `(uint lightIndex, float3 worldPos, float3 normal)`.
2. En `MainCS` de `RestirSpatial.azsl`:
   - Tras desempaquetar el reservoir del centro: re-evaluar `centerTargetPdf = RestirEvaluateTargetPdf(centerReservoir.m_lightSample.m_lightIndex, worldPos, normal)` si `m_lightIndex != 0xFFFFFFFF`; inicializar `updater.m_targetPdf = centerTargetPdf` y `updater.m_weightSum = centerTargetPdf * centerReservoir.m_W` (no `1.0`).
   - Por cada vecino que pase los filtros: `float neighborTargetPdf = RestirEvaluateTargetPdf(neighborReservoir.m_lightSample.m_lightIndex, worldPos, normal);` y `RestirMerge(updater, neighborReservoir, u, neighborTargetPdf, 1.0);` — la `similarity` deja de multiplicar pesos y pasa a ser solo gate (los `continue` ya existentes por profundidad/normal bastan; borrar el parámetro o pasar `1.0`).
3. **Vecinos aleatorios en vez de anillo fijo**: sustituir los 8 offsets fijos por `m_spatialNeighborCount = 4` vecinos en disco de radio `m_spatialRadius = 16 px`, con offset por IGN:
   ```hlsl
   float angle = RestirIgn(dispatchId.xy, PassSrg::m_frameIndex * 13 + n) * 6.2831853;
   float radius = PassSrg::m_spatialRadius * sqrt(RestirIgn(dispatchId.xy + 97, n));
   int2 neighborPixel = int2(dispatchId.xy) + int2(int2(float2(cos(angle), sin(angle)) * radius));
   ```
   Equivale a los `max_count = 4` vecinos estocásticos del resample de Capsaicin (`gi1.comp:2749-2751`), pero en screen-space.

## B.4 Fix: muestreo uniforme de candidatos en `RestirInitial.azsl`
Ahora guarda los **primeros** 8 lights del tile (sesgo por orden). Cambio: dos pasadas sobre el iterador del tile (barato, tiles de 16×16 con ≤ 256 luces):
1. Pasada 1: contar `tileLightCount` (iterar hasta `IsDone()`).
2. Pasada 2: iterar de nuevo; para cada luz, reservoir sampling de índices — o más simple: con probabilidad `RESTIR_CANDIDATE_COUNT / tileLightCount` por luz vía IGN, o elegir 8 posiciones aleatorias `u = IGN(...) * tileLightCount` y recoger las luces en esas posiciones.
3. `sourcePdf` uniforme: `1.0 / tileLightCount` por candidato (no `1.0 / candidateCount` como ahora, que era incorrecto al ser los primeros N).
Los filtros actuales se mantienen: `lightIndex < ViewSrg::m_visibleSimplePointLightCount`, descarte por `m_lightingChannelMask` si se adopta el canal dedicado de B.7, y early-out si `tileLightCount == 0` o `depth <= 0`.

## B.5 Nuevo: `RestirDebug` (cierre de B0)
Visualización sin tocar la imagen final; la textura se inspecciona con el **Pass Tool** del Editor.
`Assets/Shaders/LightCulling/RestirDebug.azsl` — compute 8×8, fullscreen dispatch:
```hlsl
ShaderResourceGroup PassSrg : SRG_PerPass
{
    Texture2D<uint4> m_reservoirInput;
    RWTexture2D<float4> m_debugOutput;
    uint m_debugMode;
}
[numthreads(8, 8, 1)]
void MainCS(uint3 dispatchId : SV_DispatchThreadID)
{
    RestirReservoir r = UnpackRestirReservoir(PassSrg::m_reservoirInput[dispatchId.xy]);
    float3 color = float3(0.0, 0.0, 0.0);
    if (PassSrg::m_debugMode == 0) { color = float3(1.0, 1.0, 1.0) * saturate(log2(1.0 + r.m_W) / 16.0); }
    else if (PassSrg::m_debugMode == 1) { color = (r.m_lightSample.m_lightIndex == 0xFFFFFFFF) ? float3(0.0, 0.0, 0.0) : frac(float3(float(r.m_lightSample.m_lightIndex) * float3(0.1031, 0.11369, 0.13787))); }
    else { color = float3(1.0, 1.0, 1.0) * saturate(r.m_M / 40.0); }
    PassSrg::m_debugOutput[dispatchId.xy] = float4(color, 1.0);
}
```
`Assets/Passes/RestirDebug.pass`: template `RestirDebugTemplate`, `PassClass: ComputePass`, slot `ReservoirInput` (Input) + `Output` con `ImageAttachment` `RGBA16_FLOAT` propio (SizeSource = ReservoirInput), `ComputePassData` con `FullscreenDispatch: true`. PassRequest dentro de `RestirParent.pass` con `"Enabled": false`, conectado a `RestirSpatial.Output`.

## B.6 Registro y build (cierre de B0)
1. `PassTemplates.azasset`: añadir `RestirParentTemplate`, `RestirInitialTemplate`, `RestirSpatialTemplate` (y `RestirDebugTemplate`) con paths `Passes/RestirParent.pass`, etc.
2. `atom_feature_common_asset_files.cmake`: listar los `.pass`, `.shader`, `.azsl`, `.azsli` (`ShaderLib/Atom/Features/LightCulling/RestirReservoir.azsli`) y `.shadervariantlist` de ReSTIR.
3. Build + Editor: el pipeline carga sin "template not found"; en el Pass Tool se ven `RestirInitial`/`RestirSpatial` bajo `OpaqueParent.Restir`; activando `RestirDebug` se ve heatmap de `W` coherente (brilla cerca de point lights).
4. **Criterio de aceptación B0**: reservoirs plausibles (W alto cerca de luces, 0xFFFFFFFF en cielo/píxeles sin luz), sin asserts de shader, coste de ambos passes < 1 ms a 1080p en la GPU de desarrollo.

## B.7 B1 — Integración con el shading: `RestirApply`
### B.7.1 Problema del double-lighting y solución por canal
El forward pass **ya evalúa todas las point lights** por material. Sumar la contribución ReSTIR encima duplicaría la luz. Solución limpia soportada por Atom: **canal de lighting dedicado**. Las luces gestionadas por ReSTIR se configuran con un `lightingChannelMask` exclusivo (p.ej. bit 2); el forward las descarta vía `IsSameLightChannel` (ya existe en todos los `ForwardPass*Lights.azsli`) y ReSTIR las trata exclusivamente:
- `RestirInitial.azsl`: filtrar candidatos con `if ((light.m_lightingChannelMask & RESTIR_LIGHTING_CHANNEL_MASK) == 0) continue;` (`#define RESTIR_LIGHTING_CHANNEL_MASK 4`).
- `RestirApply`: suma la contribución solo de esa luz.
### B.7.2 `Assets/Shaders/LightCulling/RestirApply.azsl`
Compute 8×8, fullscreen dispatch. Helpers de reconstrucción (`RestirWorldPosition`, `RestirDecodeNormalFromDepth`, `RestirIgn`) idénticos a los de los otros dos shaders (patrón actual: duplicados por shader, sin comentarios):
```hlsl
ShaderResourceGroup PassSrg : SRG_PerPass
{
    Texture2D<float> m_depthLinear;
    Texture2D<uint4> m_reservoirInput;
    RWTexture2D<float4> m_lightingInOut;
    float m_intensity;
    Sampler PointSampler { MinFilter = Point; MagFilter = Point; MipFilter = Point; AddressU = Clamp; AddressV = Clamp; AddressW = Clamp; };
}
[numthreads(8, 8, 1)]
void MainCS(uint3 dispatchId : SV_DispatchThreadID)
{
    uint2 dims;
    PassSrg::m_lightingInOut.GetDimensions(dims.x, dims.y);
    if (any(dispatchId.xy >= dims)) { return; }
    RestirReservoir reservoir = UnpackRestirReservoir(PassSrg::m_reservoirInput[dispatchId.xy]);
    if (reservoir.m_lightSample.m_lightIndex == 0xFFFFFFFF || reservoir.m_W <= 0.0) { return; }
    float2 uv = (dispatchId.xy + 0.5) / float2(dims);
    float depth = PassSrg::m_depthLinear.SampleLevel(PassSrg::PointSampler, uv, 0);
    if (depth <= 0.0) { return; }
    float3 camPos = ViewSrg::m_worldPosition.xyz;
    float3 camFwd = normalize(mul(ViewSrg::m_viewMatrixInverse, float4(0.0, 0.0, 1.0, 0.0)).xyz);
    float3 worldPos = RestirWorldPosition(uv, depth, camPos, camFwd);
    float3 normal = RestirDecodeNormalFromDepth(uv, dims, camPos, camFwd);
    SimplePointLight light = ViewSrg::m_simplePointLights[reservoir.m_lightSample.m_lightIndex];
    float3 toLight = light.m_position - worldPos;
    float distanceSq = dot(toLight, toLight);
    float3 lightDir = toLight / max(sqrt(distanceSq), 1e-6);
    float NdotL = saturate(dot(normal, lightDir));
    float falloff = saturate(distanceSq * light.m_invAttenuationRadiusSquared);
    float window = 1.0 - falloff * falloff;
    float3 contribution = light.m_rgbIntensityCandelas.rgb * NdotL * window * reservoir.m_W * PassSrg::m_intensity;
    PassSrg::m_lightingInOut[dispatchId.xy] = float4(PassSrg::m_lightingInOut[dispatchId.xy].rgb + contribution, 1.0);
}
```
Es la traducción de `PopulateCellsHandleMiss` de Capsaicin (`gi1.comp:2006`: `lighting = BRDF * light_radiance * reservoir.W`) con la BRDF difusa normalizada del target pdf (el albedo se cancela contra el target pdf de B.4 — aproximación documentada: albedo medio blanco). Sin shadow ray: las simple point lights de Atom no tienen sombras; queda como fase futura (B.9).
### B.7.3 `Assets/Passes/RestirApply.pass` y cambios en `RestirParent.pass`
`RestirApply.pass`: template `RestirApplyTemplate`, `PassClass: ComputePass`, slots `DepthLinear` (Input), `ReservoirInput` (Input), `Lighting` (InputOutput, `ScopeAttachmentUsage: Shader`), `ComputePassData` con `FullscreenDispatch: true`.
`RestirParent.pass`: añadir slots `Modulate` (Input) y `LightingOutput` (Output); `LightingOutput` conectado a `RestirApply.Lighting`; PassRequest `RestirApply` con `ReservoirInput ← RestirSpatial.Output`, `Lighting ← Parent.Modulate`, `DepthLinear ← Parent.DepthLinear`.
### B.7.4 Hookup en `OpaqueParent.pass`
En el PassRequest "Restir" existente (líneas 574-599) añadir la conexión `Modulate ← SubsurfaceScatteringPass.Output`, y cambiar el slot `Modulate` del PassRequest "Ssao" (líneas 604-609) para que lea de `Restir.LightingOutput` en vez de `SubsurfaceScatteringPass`. Cadena final: `SubsurfaceScattering → Restir → Ssao → Ssgi → RTShadowComposite`.
### B.7.5 C++: constantes en `RestirComputePass`
Sustituir el `uint m_frameIndex` suelto por un struct de constantes en los PassSrg de los 4 shaders (patrón de `SsgiComputePass`):
```cpp
struct RestirConstants
{
    float m_intensity = 1.0f;
    float m_spatialRadius = 16.0f;
    uint32_t m_spatialNeighborCount = 4;
    uint32_t m_debugMode = 0;
    uint32_t m_frameIndex = 0;
    float m_padding[3] = { 0.0f, 0.0f, 0.0f };
};
```
`FrameBeginInternal` setea el struct completo (incrementando `m_frameIndex & 63`), igual que `SsgiComputePass::FrameBeginInternal`. Tuning posterior vía pass settings o consola; defaults arriba.
### B.7.6 Criterio de aceptación B1
Escena de prueba: 20-50 point lights con `lightingChannelMask = 4`. El forward no las aplica; `RestirApply` las ilumina con ruido característico de ReSTIR que desaparece tras el spatial reuse en zonas planas; `m_intensity = 0` devuelve la imagen base; el Pass Tool muestra `W` alto solo junto a luces.

## B.8 Orden de ejecución resumido
1. **B0.1** Registrar 6-7 templates en `PassTemplates.azasset` (Ssgi×3 + Restir×3-4) + assets en `atom_feature_common_asset_files.cmake` → el pipeline actual ya carga.
2. **B0.2** Fix shift mapping (B.3) + muestreo uniforme (B.4) + mover `RestirEvaluateTargetPdf` a `RestirReservoir.azsli`.
3. **B0.3** `RestirDebug` (B.5) + verificación en Editor (B.6).
4. **B1.1** `RestirApply.azsl/.pass` + cambios en `RestirParent.pass` + hookup en `OpaqueParent.pass` (B.7.2-B.7.4).
5. **B1.2** Constantes en `RestirComputePass` (B.7.5) + build + aceptación (B.7.6).

## B.9 Fases futuras (fuera de scope, documentadas)
- **Temporal reuse**: ping-pong de reservoirs entre frames (buffer persistente creado en C++ con `AttachBufferToSlot`, patrón de `LightCullingRemap.cpp:109-125`), reproyección con depth/normal del frame anterior y `RestirClampM(20)` — es el `ResampleReservoirs` de Capsaicin.
- **Spot lights con sombra**: `ProjectedShadow::GetVisibility` (`Atom/Features/Shadow/ProjectedShadow.azsli:36`), requiere bindear `m_projectedShadowmaps` en el PassSrg.
- **Area lights**: `m_quadLights`/`m_polygonLights` del ViewSrg con `sampleParams` (barycentrics) igual que `sampleAreaLightAM` de Capsaicin.
- **MIS pairwise en update** y **Talbot MIS en merge** (`mergeReservoirsTalbotMIS`, `reservoir.hlsl:533`) si el bias temporal/espacial lo justifica.
- **Hash grid world-space** estilo GI1 (`world_space_restir.hlsl`) si screen-space no basta para volumétricos/partículas.
