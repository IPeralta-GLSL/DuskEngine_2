#include <ScreensCtrl.h>
#include <ScreenFactory.h>
#include <ScreenWidget.h>
#include <UpdateProjectCtrl.h>

#include <QPainter>
#include <QProxyStyle>
#include <QStyleOptionTab>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

namespace O3DE::ProjectManager
{
    class HorizontalTabStyle : public QProxyStyle
    {
    public:
        using QProxyStyle::QProxyStyle;

        QSize sizeFromContents(ContentsType type, const QStyleOption* option,
            const QSize& size, const QWidget* widget) const override
        {
            QSize s = QProxyStyle::sizeFromContents(type, option, size, widget);
            if (type == QStyle::CT_TabBarTab)
            {
                s.transpose();
                s.rwidth() = 160;
                s.rheight() = 44;
            }
            return s;
        }

        void drawControl(ControlElement element, const QStyleOption* option,
            QPainter* painter, const QWidget* widget) const override
        {
            if (element == CE_TabBarTabLabel)
            {
                if (const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(option))
                {
                    QStyleOptionTab adjustedTab = *tab;
                    adjustedTab.shape = QTabBar::RoundedNorth;
                    QProxyStyle::drawControl(element, &adjustedTab, painter, widget);
                    return;
                }
            }
            QProxyStyle::drawControl(element, option, painter, widget);
        }
    };
    ScreensCtrl::ScreensCtrl(QWidget* parent, DownloadController* downloadController)
        : QWidget(parent)
        , m_downloadController(downloadController)
    {
        setObjectName("ScreensCtrl");

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(0, 0, 0, 0);
        setLayout(vLayout);

        m_screenStack = new QStackedWidget();
        vLayout->addWidget(m_screenStack);

        m_tabWidget = new QTabWidget();
        m_tabWidget->setTabPosition(QTabWidget::West);
        m_tabWidget->tabBar()->setStyle(new HorizontalTabStyle(m_tabWidget->tabBar()->style()));
        m_tabWidget->tabBar()->setFocusPolicy(Qt::TabFocus);
        m_screenStack->addWidget(m_tabWidget);
        connect(m_tabWidget, &QTabWidget::currentChanged, this, &ScreensCtrl::TabChanged);
    }

    void ScreensCtrl::BuildScreens(QVector<ProjectManagerScreen> screens)
    {
        for (ProjectManagerScreen screen : screens)
        {
            ResetScreen(screen);
        }
    }

    ScreenWidget* ScreensCtrl::FindScreen(ProjectManagerScreen screen)
    {
        const auto iterator = m_screenMap.find(screen);
        if (iterator != m_screenMap.end())
        {
            return iterator.value();
        }
        return nullptr;
    }

    ScreenWidget* ScreensCtrl::GetCurrentScreen()
    {
        if (m_screenStack->currentWidget() == m_tabWidget)
        {
            return static_cast<ScreenWidget*>(m_tabWidget->currentWidget());
        }
        return static_cast<ScreenWidget*>(m_screenStack->currentWidget());
    }

    bool ScreensCtrl::ChangeToScreen(ProjectManagerScreen screen)
    {
        if (m_screenStack->currentWidget())
        {
            ScreenWidget* currentScreenWidget = GetCurrentScreen();
            if (currentScreenWidget->IsReadyForNextScreen())
            {
                return ForceChangeToScreen(screen);
            }
        }
        return false;
    }

    bool ScreensCtrl::ForceChangeToScreen(ProjectManagerScreen screen, bool addVisit)
    {
        ScreenWidget* newScreen = nullptr;

        const auto iterator = m_screenMap.find(screen);
        if (iterator != m_screenMap.end())
        {
            newScreen = iterator.value();
        }
        else
        {
            for (ScreenWidget* checkingScreen : m_screenMap)
            {
                if (checkingScreen->ContainsScreen(screen))
                {
                    newScreen = checkingScreen;
                    break;
                }
            }
        }

        if (!newScreen)
        {
            return false;
        }

        ScreenWidget* currentScreen = GetCurrentScreen();

        if (currentScreen != newScreen)
        {
            if (addVisit)
            {
                m_screenVisitOrder.push(currentScreen->GetScreenEnum());
            }

            if (newScreen->IsTab())
            {
                m_tabWidget->setCurrentWidget(newScreen);
                m_screenStack->setCurrentWidget(m_tabWidget);
            }
            else
            {
                m_screenStack->setCurrentWidget(newScreen);
            }

            newScreen->NotifyCurrentScreen();

            if (iterator == m_screenMap.end())
            {
                newScreen->GoToScreen(screen);
            }

            return true;
        }

        newScreen->NotifyCurrentScreen();
        return false;
    }

    bool ScreensCtrl::GoToPreviousScreen()
    {
        if (!m_screenVisitOrder.isEmpty())
        {
            ProjectManagerScreen previousScreen = m_screenVisitOrder.pop();
            return ForceChangeToScreen(previousScreen, false);
        }
        return false;
    }

    void ScreensCtrl::ResetScreen(ProjectManagerScreen screen)
    {
        bool shouldRestoreCurrentScreen = GetCurrentScreen() && GetCurrentScreen()->GetScreenEnum() == screen;
        int tabIndex = GetScreenTabIndex(screen);

        DeleteScreen(screen);

        ScreenWidget* newScreen = BuildScreen(this, screen, m_downloadController);
        if (newScreen->IsTab())
        {
            if (tabIndex > -1)
            {
                m_tabWidget->insertTab(tabIndex, newScreen, newScreen->GetTabText());
            }
            else
            {
                m_tabWidget->addTab(newScreen, newScreen->GetTabText());
            }
            if (shouldRestoreCurrentScreen)
            {
                m_tabWidget->setCurrentWidget(newScreen);
                m_screenStack->setCurrentWidget(m_tabWidget);
                newScreen->NotifyCurrentScreen();
            }
        }
        else
        {
            m_screenStack->addWidget(newScreen);
            if (shouldRestoreCurrentScreen)
            {
                m_screenStack->setCurrentWidget(newScreen);
                newScreen->NotifyCurrentScreen();
            }
        }

        m_screenMap.insert(screen, newScreen);

        connect(newScreen, &ScreenWidget::ChangeScreenRequest, this, &ScreensCtrl::ChangeToScreen);
        connect(newScreen, &ScreenWidget::GoToPreviousScreenRequest, this, &ScreensCtrl::GoToPreviousScreen);
        connect(newScreen, &ScreenWidget::ResetScreenRequest, this, &ScreensCtrl::ResetScreen);
        connect(newScreen, &ScreenWidget::NotifyCurrentProject, this, &ScreensCtrl::NotifyCurrentProject);
        connect(newScreen, &ScreenWidget::NotifyBuildProject, this, &ScreensCtrl::NotifyBuildProject);
        connect(newScreen, &ScreenWidget::NotifyProjectRemoved, this, &ScreensCtrl::NotifyProjectRemoved);
    }

    void ScreensCtrl::ResetAllScreens()
    {
        for (auto iter = m_screenMap.begin(); iter != m_screenMap.end(); ++iter)
        {
            ResetScreen(iter.key());
        }
    }

    void ScreensCtrl::DeleteScreen(ProjectManagerScreen screen)
    {
        const auto iter = m_screenMap.find(screen);
        if (iter == m_screenMap.end())
        {
            return;
        }

        ScreenWidget* screenToDelete = iter.value();
        if (screenToDelete->IsTab())
        {
            int tabIndex = m_tabWidget->indexOf(screenToDelete);
            if (tabIndex > -1)
            {
                m_tabWidget->removeTab(tabIndex);
            }
        }
        else
        {
            m_screenStack->removeWidget(screenToDelete);
        }

        m_screenMap.erase(iter);
    }

    void ScreensCtrl::DeleteAllScreens()
    {
        for (auto iter = m_screenMap.begin(); iter != m_screenMap.end(); ++iter)
        {
            DeleteScreen(iter.key());
        }
    }

    void ScreensCtrl::TabChanged([[maybe_unused]] int index)
    {
        ScreenWidget* screen = static_cast<ScreenWidget*>(m_tabWidget->currentWidget());
        if (screen)
        {
            screen->NotifyCurrentScreen();
        }
    }

    int ScreensCtrl::GetScreenTabIndex(ProjectManagerScreen screen)
    {
        const auto iter = m_screenMap.find(screen);
        if (iter != m_screenMap.end())
        {
            ScreenWidget* screenWidget = iter.value();
            if (screenWidget->IsTab())
            {
                return m_tabWidget->indexOf(screenWidget);
            }
        }
        return -1;
    }
}
