/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QMetaType>
AZ_POP_DISABLE_WARNING

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Variable/VariableCore.h>
// AZ::Uuid and AZ::Data::AssetId are auto-registered with QVariant in Qt 6.8+,
// which avoids "explicit specialization after instantiation" errors in unity builds.
Q_DECLARE_METATYPE(ScriptCanvas::Data::Type);
Q_DECLARE_METATYPE(ScriptCanvas::VariableId);
Q_DECLARE_METATYPE(ScriptCanvas::SourceHandle);

