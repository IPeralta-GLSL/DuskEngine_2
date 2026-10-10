#pragma once

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>

namespace AzToolsFramework
{
    namespace UIAxis
    {
        inline AZ::Vector3 ToDisplay(const AZ::Vector3& value)
        {
            return AZ::Vector3(value.GetX(), value.GetZ(), value.GetY());
        }

        inline AZ::Vector3 FromDisplay(const AZ::Vector3& value)
        {
            return AZ::Vector3(value.GetX(), value.GetZ(), value.GetY());
        }

        inline AZ::Quaternion ToDisplay(const AZ::Quaternion& value)
        {
            AZ::Matrix3x3 swap = AZ::Matrix3x3::CreateIdentity();
            swap.SetRow(1, AZ::Vector3(0.0f, 0.0f, 1.0f));
            swap.SetRow(2, AZ::Vector3(0.0f, 1.0f, 0.0f));
            return AZ::Quaternion::CreateFromMatrix3x3(swap * AZ::Matrix3x3::CreateFromQuaternion(value) * swap);
        }

        inline AZ::Quaternion FromDisplay(const AZ::Quaternion& value)
        {
            return ToDisplay(value);
        }
    }
}
