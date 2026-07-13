/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QtGlobal>
#include <QRect>
#include <QWidget>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    #define O3DE_QT6 1
#else
    #define O3DE_QT5 1
#endif

#ifdef O3DE_QT6
    #include <QEnterEvent>
    #define O3DE_ENTER_EVENT_TYPE QEnterEvent
#else
    #define O3DE_ENTER_EVENT_TYPE QEvent
#endif

#ifdef O3DE_QT6
    #define O3DE_MOUSE_POS(event) (event)->position().toPoint()
    #define O3DE_MOUSE_GLOBAL_POS(event) (event)->globalPosition().toPoint()
#else
    #define O3DE_MOUSE_POS(event) (event)->pos()
    #define O3DE_MOUSE_GLOBAL_POS(event) (event)->globalPos()
#endif

#ifdef O3DE_QT6
    #define O3DE_MIDDLE_BUTTON Qt::MiddleButton
#else
    #define O3DE_MIDDLE_BUTTON Qt::MidButton
#endif

#ifdef O3DE_QT6
    #include <QScreen>
    #include <QGuiApplication>
    inline QRect O3DE_AvailableGeometry(QWidget* widget)
    {
        QScreen* screen = widget->screen();
        return screen ? screen->availableGeometry() : QRect();
    }
#else
    #include <QDesktopWidget>
    #include <QApplication>
    inline QRect O3DE_AvailableGeometry(QWidget* widget)
    {
        return QApplication::desktop()->availableGeometry(widget);
    }
#endif

#ifdef O3DE_QT6
    #include <QStringView>
    #define O3DE_MIDREF(str, pos, len) (str).mid(pos, len)
    #define O3DE_RIGHTREF(str, len) (str).right(len)
#else
    #include <QStringRef>
    #define O3DE_MIDREF(str, pos, len) (str).midRef(pos, len)
    #define O3DE_RIGHTREF(str, len) (str).rightRef(len)
#endif

#ifdef O3DE_QT6
    #define O3DE_STYLEOPTION_VIEWITEM QStyleOptionViewItem
#else
    #define O3DE_STYLEOPTION_VIEWITEM QStyleOptionViewItemV4
#endif

#define O3DE_SUPPRESS_QT6_DEPRECATED_WARNINGS_BEGIN \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")

#define O3DE_SUPPRESS_QT6_DEPRECATED_WARNINGS_END \
    _Pragma("GCC diagnostic pop")
