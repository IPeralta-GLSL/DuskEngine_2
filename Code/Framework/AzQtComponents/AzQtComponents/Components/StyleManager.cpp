/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <QTextStream>
#include <QApplication>
#include <QColor>
#include <QPalette>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <QDir>
AZ_POP_DISABLE_WARNING
#include <QString>
#include <QFile>
#include <QFontDatabase>
#include <QStyleFactory>
#include <QPointer>
#include <QStyle>
#include <QWidget>
#include <QDebug>

#include <AzQtComponents/Components/StylesheetPreprocessor.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzQtComponents/Components/StyleSheetCache.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>
#include <AzQtComponents/Components/AutoCustomWindowDecorations.h>

namespace AzQtComponents
{

    constexpr QStringView g_styleSheetRelativePath {u"Code/Framework/AzQtComponents/AzQtComponents/Components/Widgets"};
    constexpr QStringView g_styleSheetResourcePath {u":AzQtComponents/Widgets"};
    constexpr QStringView g_globalStyleSheetName {u"BaseStyleSheet.qss"};
    constexpr QStringView g_searchPathPrefix {u"AzQtComponentWidgets"};

    StyleManager* StyleManager::s_instance = nullptr;

    static QStyle* createBaseStyle()
    {
        return QStyleFactory::create("Fusion");

    }

    void StyleManager::addSearchPaths(const QString& searchPrefix, const QString& pathOnDisk, const QString& qrcPrefix,
        const AZ::IO::PathView& engineRootPath)
    {
        if (!s_instance)
        {
            qFatal("StyleManager::addSearchPaths called before instance was created");
            return;
        }

        s_instance->m_stylesheetCache->addSearchPaths(searchPrefix, pathOnDisk, qrcPrefix, engineRootPath);
    }

    bool StyleManager::setStyleSheet(QWidget* widget, QString styleFileName)
    {
        if (!s_instance)
        {
            qFatal("StyleManager::setStyleSheet called before instance was created");
            return false;
        }

        if (!widget)
        {
            qFatal("StyleManager::setStyleSheet called with null widget pointer");
            return false;
        }

        if (!styleFileName.endsWith(StyleSheetCache::styleSheetExtension()))
        {
            styleFileName.append(StyleSheetCache::styleSheetExtension());
        }

        const auto styleSheet = s_instance->m_stylesheetCache->loadStyleSheet(styleFileName);
        if (styleSheet.isEmpty())
        {
            return false;
        }

        s_instance->m_widgetToStyleSheetMap.insert(widget, styleFileName);

        connect(widget, &QObject::destroyed, s_instance, &StyleManager::stopTrackingWidget, Qt::UniqueConnection);

        widget->setStyleSheet(styleSheet);

        return true;
    }

    QStyle *StyleManager::baseStyle(const QWidget*)
    {
        return qApp->style();
    }

    void StyleManager::repolishStyleSheet(QWidget* widget)
    {
        if (widget)
        {
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
        }
    }

    StyleManager::StyleManager(QObject* parent)
        : QObject(parent)
        , m_stylesheetPreprocessor(new StylesheetPreprocessor(this))
        , m_stylesheetCache(new StyleSheetCache(this))
    {
        if (s_instance)
        {
            qFatal("A StyleManager already exists");
        }

        if (QApplication* app = qobject_cast<QApplication*>(QCoreApplication::instance()))
        {
            QPalette palette;
            palette.setColor(QPalette::Window, QColor("#1f1f22"));
            palette.setColor(QPalette::WindowText, QColor("#C9C9CE"));
            palette.setColor(QPalette::Base, QColor("#1f1f22"));
            palette.setColor(QPalette::AlternateBase, QColor("#232327"));
            palette.setColor(QPalette::Text, QColor("#C9C9CE"));
            palette.setColor(QPalette::Button, QColor("#1f1f22"));
            palette.setColor(QPalette::ButtonText, QColor("#C9C9CE"));
            palette.setColor(QPalette::PlaceholderText, QColor("#8E8E94"));
            palette.setColor(QPalette::Highlight, QColor("#4CAF50"));
            palette.setColor(QPalette::HighlightedText, QColor("#1f1f22"));
            palette.setColor(QPalette::ToolTipBase, QColor("#1f1f22"));
            palette.setColor(QPalette::ToolTipText, QColor("#C9C9CE"));
            palette.setColor(QPalette::Link, QColor("#4CAF50"));
            palette.setColor(QPalette::Dark, QColor("#1c1c1f"));
            palette.setColor(QPalette::Mid, QColor("#1f1f22"));
            palette.setColor(QPalette::Midlight, QColor("#232327"));
            palette.setColor(QPalette::Light, QColor("#2C2C31"));
            palette.setColor(QPalette::Shadow, QColor("#34343A"));
            palette.setColor(QPalette::BrightText, QColor("#F2F2F4"));
            palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#5A5A60"));
            palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#5A5A60"));
            palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#5A5A60"));
            app->setPalette(palette);
        }
    }

    StyleManager::~StyleManager()
    {
        delete m_stylesheetPreprocessor;
        s_instance = nullptr;

        if (m_style)
        {
            delete m_style.data();
            m_style.clear();
        }
    }

    void StyleManager::initialize([[maybe_unused]] QApplication* application, const AZ::IO::PathView& engineRootPath)
    {
        if (s_instance)
        {
            qFatal("StyleManager::Initialize called more than once");
            return;
        }
        s_instance = this;

        connect(application, &QCoreApplication::aboutToQuit, this, &StyleManager::cleanupStyles);

        initializeSearchPaths(application, engineRootPath);
        initializeFonts();

        QFont defaultFont("Manrope");
        defaultFont.setPixelSize(12);
        QApplication::setFont(defaultFont);

        m_titleBarOverdrawHandler = TitleBarOverdrawHandler::createHandler(application, this);

        m_autoCustomWindowDecorations = new AutoCustomWindowDecorations(this);
        m_autoCustomWindowDecorations->setMode(AutoCustomWindowDecorations::Mode_AnyWindow);

        m_style = new Style(createBaseStyle());

        QApplication::setStyle(m_style);
        m_style->setParent(this);
        refresh();

        connect(m_stylesheetCache, &StyleSheetCache::styleSheetsChanged, this, [this]
        {
            refresh();
        });
    }

    void StyleManager::cleanupStyles()
    {
        QApplication::setStyle(createBaseStyle());
    }

    void StyleManager::stopTrackingWidget(QObject* object)
    {
        const auto widget = qobject_cast<QWidget* const>(object);
        if (!widget)
        {
            return;
        }

        m_widgetToStyleSheetMap.remove(widget);

        widget->setStyleSheet(QString());
    }

    void StyleManager::initializeFonts()
    {
        QString openSansPathSpecifier = QStringLiteral(":/AzQtFonts/Fonts/Open_Sans/%1");
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Bold.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-BoldItalic.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-ExtraBold.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-ExtraBoldItalic.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Italic.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Light.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-LightItalic.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Regular.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Semibold.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-SemiboldItalic.ttf"));
    }

    void StyleManager::initializeSearchPaths([[maybe_unused]] QApplication* application, const AZ::IO::PathView& engineRootPath)
    {
        QString rootDir = QString::fromUtf8(engineRootPath.Native().data(), aznumeric_cast<int>(engineRootPath.Native().size()));

        if (!rootDir.isEmpty())
        {
            QDir appPath(rootDir);

            const auto pathOnDisk = appPath.absoluteFilePath(g_styleSheetRelativePath.toString());
            m_stylesheetCache->setFallbackSearchPaths(g_searchPathPrefix.toString(), pathOnDisk, g_styleSheetResourcePath.toString());

            QDir::addSearchPath("STYLESHEETIMAGES", appPath.filePath("Assets/Editor/Styles/StyleSheetImages"));
            QDir::addSearchPath("UI", appPath.filePath("Assets/Editor/UI"));
            QDir::addSearchPath("EDITOR", appPath.filePath("Assets/Editor"));
        }
    }

    void StyleManager::refresh()
    {
        const auto globalStyleSheet = m_stylesheetCache->loadStyleSheet(g_globalStyleSheetName.toString());
        qApp->setStyleSheet(globalStyleSheet);

        auto i = m_widgetToStyleSheetMap.constBegin();
        while (i != m_widgetToStyleSheetMap.constEnd())
        {
            const auto styleSheet = m_stylesheetCache->loadStyleSheet(i.value());
            i.key()->setStyleSheet(styleSheet);
            ++i;
        }

        QFont titleBarFont("Manrope");
        titleBarFont.setPixelSize(18);
        QApplication::setFont(titleBarFont, "QMdiSubWindowTitleBar");
    }

    const QColor& StyleManager::getColorByName(const QString& name)
    {
        return m_stylesheetPreprocessor->GetColorByName(name);
    }
} // namespace AzQtComponents

#include "Components/moc_StyleManager.cpp"

#if defined(AZ_QT_COMPONENTS_STATIC)
#include <Components/rcc_resources.h>
#endif // #if defined(AZ_QT_COMPONENTS_STATIC)