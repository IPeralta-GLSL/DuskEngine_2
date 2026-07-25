/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

AZ_GFX_VEC3_PARAM(LightColor, m_lightColor, Vector3(1.0f, 0.9f, 0.7f))

AZ_GFX_FLOAT_PARAM(LightIntensity, m_lightIntensity, 1.0f)
AZ_GFX_FLOAT_PARAM(Density, m_density, 0.05f)
AZ_GFX_FLOAT_PARAM(Scattering, m_scattering, 3.0f)
AZ_GFX_FLOAT_PARAM(Extinction, m_extinction, 0.5f)
AZ_GFX_FLOAT_PARAM(Anisotropy, m_anisotropy, 0.4f)
AZ_GFX_FLOAT_PARAM(Steps, m_steps, 32.0f)
AZ_GFX_FLOAT_PARAM(MaxDistance, m_maxDistance, 200.0f)
AZ_GFX_FLOAT_PARAM(StartDistance, m_startDistance, 0.5f)
AZ_GFX_FLOAT_PARAM(FogMinHeight, m_fogMinHeight, -100.0f)
AZ_GFX_FLOAT_PARAM(FogMaxHeight, m_fogMaxHeight, 100.0f)
AZ_GFX_FLOAT_PARAM(NoiseScale, m_noiseScale, 0.01f)
AZ_GFX_FLOAT_PARAM(NoiseSpeed, m_noiseSpeed, 0.1f)
AZ_GFX_FLOAT_PARAM(NoiseStrength, m_noiseStrength, 0.5f)
AZ_GFX_FLOAT_PARAM(Time, m_time, 0.0f)
AZ_GFX_BOOL_PARAM(EnableShadows, m_enableShadows, true)
AZ_GFX_BOOL_PARAM(Enabled, m_enabled, false)
