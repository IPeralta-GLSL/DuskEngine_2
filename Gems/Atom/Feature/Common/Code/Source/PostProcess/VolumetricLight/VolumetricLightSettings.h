/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightSettingsInterface.h>
#include <PostProcess/PostProcessBase.h>

namespace AZ
{
    namespace Render
    {
        class PostProcessSettings;

        class VolumetricLightSettings final
            : public VolumetricLightSettingsInterface
            , public PostProcessBase
        {
            friend class VolumetricLightPass;
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI(AZ::Render::VolumetricLightSettings, "{C7F3A8B2-5D4E-4F1A-9C6B-2E7D5A1F3C8B}",
                AZ::Render::VolumetricLightSettingsInterface, AZ::Render::PostProcessBase);
            AZ_CLASS_ALLOCATOR(AZ::Render::VolumetricLightSettings, SystemAllocator);

            VolumetricLightSettings(PostProcessFeatureProcessor* featureProcessor);
            VolumetricLightSettings();
            ~VolumetricLightSettings() = default;

            void OnSettingsChanged() override;
            bool GetSettingsNeedUpdate() { return m_needUpdate; }
            void SetSettingsNeedUpdate(bool needUpdate) { m_needUpdate = needUpdate; }

            void SetInitialized(bool isInitialized) override { m_isInitialized = isInitialized; }
            bool IsInitialized() override { return m_isInitialized; }

            void Simulate([[maybe_unused]] float deltaTime) {}
            void ApplySettingsTo(VolumetricLightSettings* target, float alpha) const;

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
            ValueType Get##Name() const override;                                                           \
            void Set##Name(ValueType val) override;                                                         \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
            ValueType MemberName = DefaultValue;                                                            \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
            RHI::ShaderInputConstantIndex MemberName##SrgIndex;                                             \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/PostProcess/VolumetricLight/VolumetricLightParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            PostProcessSettings* m_parentSettings = nullptr;
            bool m_isInitialized = false;
            bool m_needUpdate = true;
        };
    }
}
