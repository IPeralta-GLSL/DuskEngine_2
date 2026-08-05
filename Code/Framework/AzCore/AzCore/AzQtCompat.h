#pragma once

#include <QtGlobal>
#include <QRect>
#include <QWidget>
#include <QEnterEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QStringView>

#define O3DE_QT6 1

#define O3DE_ENTER_EVENT_TYPE QEnterEvent
#define O3DE_MOUSE_POS(event) (event)->position().toPoint()
#define O3DE_MOUSE_GLOBAL_POS(event) (event)->globalPosition().toPoint()
#define O3DE_MIDDLE_BUTTON Qt::MiddleButton

inline QRect O3DE_AvailableGeometry(QWidget* widget)
{
    QScreen* screen = widget->screen();
    return screen ? screen->availableGeometry() : QRect();
}

#define O3DE_MIDREF(str, pos, len) (str).mid(pos, len)
#define O3DE_RIGHTREF(str, len) (str).right(len)

#define O3DE_STYLEOPTION_VIEWITEM QStyleOptionViewItem

#define O3DE_SUPPRESS_QT6_DEPRECATED_WARNINGS_BEGIN \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")

#define O3DE_SUPPRESS_QT6_DEPRECATED_WARNINGS_END \
    _Pragma("GCC diagnostic pop")
