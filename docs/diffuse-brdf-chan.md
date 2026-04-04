# Diffuse BRDF: Lambert → Chan (Hammon GDC2017)

## Summary

The engine's diffuse lighting model was replaced from simple **Lambertian** to the roughness-aware **Hammon/Chan diffuse BRDF** (Earl Hammon Jr., "PBR Diffuse Lighting for GGX+Smith Microsurfaces", GDC 2017). This is the same model used by Frostbite and Pixar's RenderMan.

---

## Why Replace Lambert?

Lambertian diffuse has two well-known physical inaccuracies:

1. **Ignores surface roughness.** A rough concrete wall and a smooth wax surface produce the same diffuse response, which is physically wrong.
2. **Overbright at grazing angles.** Lambert produces unrealistically bright edges on surfaces, especially noticeable on organic forms like skin and fabric.

The Hammon model corrects both by:
- Interpolating between a **smooth lobe** (double-Fresnel, Disney-like, for low roughness) and a **rough retro-reflection lobe** (empirical, for high roughness).
- Adding a **multi-scattering compensation** term that recovers energy lost by the single-scattering approximation.

---

## New Function: `DiffuseChan`

**File:** `Gems/Atom/Feature/Common/Assets/ShaderLib/Atom/Features/PBR/Microfacet/Brdf.azsli`

```hlsl
//! Roughness-aware diffuse BRDF (Hammon, GDC 2017).
real3 DiffuseChan(real roughnessA, real3 albedo, real3 normal, real3 dirToLight, real3 dirToCamera, real3 diffuseResponse)
{
    real NdotL = saturate(dot(normal, dirToLight));
    real NdotV = saturate(dot(normal, dirToCamera));
    real LdotV = dot(dirToLight, dirToCamera);
    real LplusV_LenSq = 2.0 + 2.0 * LdotV;
    real NdotH = (NdotL + NdotV) * rsqrt(max(LplusV_LenSq, 1e-5));
    real facing = LplusV_LenSq * 0.25;
    real rough = facing * (0.9 - 0.4 * facing) * ((0.5 + NdotH) / max(NdotH, 1e-5));
    real smooth = 1.05 * (1.0 - pow(1.0 - NdotL, 5.0)) * (1.0 - pow(1.0 - NdotV, 5.0));
    real single = INV_PI * lerp(smooth, rough, roughnessA);
    real multi = 0.1159 * roughnessA;
    return albedo * (single + albedo * multi) * NdotL * diffuseResponse;
}
```

The old `DiffuseLambertian` function is kept in the file but is no longer called anywhere.

---

## Files Changed

### 1. `Brdf.azsli` — New function added

Added `DiffuseChan` after `DiffuseLambertian`.

---

### 2. `StandardLighting.azsli` — Main PBR pipeline

**File:** `Gems/Atom/Feature/Common/Assets/ShaderLib/Atom/Features/PBR/Lighting/StandardLighting.azsli`

```hlsl
// Before
real3 diffuse = DiffuseLambertian(surface.albedo, surface.GetDiffuseNormal(), dirToLight, lightingData.diffuseResponse);

// After
real3 diffuse = DiffuseChan(surface.roughnessA, surface.albedo, surface.GetDiffuseNormal(), dirToLight, lightingData.dirToCamera[0], lightingData.diffuseResponse);
```

---

### 3. `FallbackPBRMaterial.azsli` — Fallback material pipeline

**File:** `Gems/Atom/Feature/Common/Assets/ShaderLib/Atom/Features/FallbackPBRMaterial/FallbackPBRMaterial.azsli`

Same substitution as StandardLighting.

---

### 4. `BasePBR_LightingBrdf.azsli` — Base PBR material type

**File:** `Gems/Atom/Feature/Common/Assets/Shaders/Materials/BasePBR/BasePBR_LightingBrdf.azsli`

```hlsl
// Before
real3 diffuse = DiffuseLambertian(surface.albedo, surface.GetDiffuseNormal(), dirToLight, lightingData.diffuseResponse);

// After
real3 diffuse = DiffuseChan(surface.roughnessA, surface.albedo, surface.GetDiffuseNormal(), dirToLight, lightingData.dirToCamera[0], lightingData.diffuseResponse);
```

---

### 5. `StandardPBR_LightingBrdf.azsli` — Standard PBR material type

**File:** `Gems/Atom/Feature/Common/Assets/Shaders/Materials/StandardPBR/StandardPBR_LightingBrdf.azsli`

Same substitution as BasePBR.

---

### 6. `EnhancedPBR_LightingBrdf.azsli` — Enhanced PBR material type

**File:** `Gems/Atom/Feature/Common/Assets/Shaders/Materials/EnhancedPBR/EnhancedPBR_LightingBrdf.azsli`

Only the non-SSS branch was changed. When `o_enableSubsurfaceScattering` is active, `NormalizedDisneyDiffuse` is still used intentionally (it models the double-Fresnel enter/exit behavior required for subsurface materials).

```hlsl
// Before (non-SSS branch)
diffuse = DiffuseLambertian(surface.albedo, surface.GetDiffuseNormal(), dirToLight, lightingData.diffuseResponse);

// After
diffuse = DiffuseChan(surface.roughnessA, surface.albedo, surface.GetDiffuseNormal(), dirToLight, lightingData.dirToCamera[0], lightingData.diffuseResponse);
```

---

### 7. `ReflectionScreenSpaceRayTracingCommon.azsli` — Ray tracing light loop

**File:** `Gems/Atom/Feature/Common/Assets/Shaders/Reflections/ReflectionScreenSpaceRayTracingCommon.azsli`

`m_roughnessA` was added to `RayTracingLightingData`:

```hlsl
// Before
struct RayTracingLightingData
{
    ...
    float  m_roughnessA2;
    ...
};

// After
struct RayTracingLightingData
{
    ...
    float  m_roughnessA;
    float  m_roughnessA2;
    ...
};
```

All 9 `DiffuseLambertian` call sites inside this file (directional, point, spot, disk, rectangle lights) were updated:

```hlsl
// Before
diffuse += DiffuseLambertian(lightingData.m_albedo, lightingData.m_normal, dirToLight, lightingData.m_diffuseResponse) * lightIntensity;

// After
diffuse += DiffuseChan(lightingData.m_roughnessA, lightingData.m_albedo, lightingData.m_normal, dirToLight, lightingData.m_dirToCamera, lightingData.m_diffuseResponse) * lightIntensity;
```

---

### 8. `ReflectionScreenSpaceRayTracingClosestHit.azsli` — Ray tracing hit shader

**File:** `Gems/Atom/Feature/Common/Assets/Shaders/Reflections/ReflectionScreenSpaceRayTracingClosestHit.azsli`

Initializes the new field:

```hlsl
// Before
rayTracingLightingData.m_roughnessA2 = roughnessA2;

// After
rayTracingLightingData.m_roughnessA  = roughnessA;
rayTracingLightingData.m_roughnessA2 = roughnessA2;
```

---

## Scope and Limitations

| Domain | Status |
|---|---|
| Direct lighting (all light types) | ✅ Chan diffuse |
| Ray-traced reflections (direct lights) | ✅ Chan diffuse |
| Image-Based Lighting (IBL / diffuse env map) | ⚠️ Not affected — IBL samples a pre-integrated irradiance map that encodes a Lambertian integral baked at asset creation time. This is standard practice in all real-time engines including Frostbite. |
| Subsurface scattering (EnhancedPBR) | ✅ Intentionally kept as `NormalizedDisneyDiffuse`, which models the double-Fresnel scattering specific to SSS materials. |

---

## Reference

Earl Hammon Jr., "PBR Diffuse Lighting for GGX+Smith Microsurfaces", GDC 2017, pp. 113–135.
