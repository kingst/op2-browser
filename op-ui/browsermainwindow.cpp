/*======================================================== 
**University of Illinois/NCSA 
**Open Source License 
**
**Copyright (C) 2007-2008,The Board of Trustees of the University of 
**Illinois. All rights reserved. 
**
**Developed by: 
**
**    Research Group of Professor Sam King in the Department of Computer 
**    Science The University of Illinois at Urbana-Champaign 
**    http://www.cs.uiuc.edu/homes/kingst/Research.html 
**
**Permission is hereby granted, free of charge, to any person obtaining a 
**copy of this software and associated documentation files (the 
**��Software��), to deal with the Software without restriction, including 
**without limitation the rights to use, copy, modify, merge, publish, 
**distribute, sublicense, and/or sell copies of the Software, and to 
**permit persons to whom the Software is furnished to do so, subject to 
**the following conditions: 
**
*** Redistributions of source code must retain the above copyright notice, 
**this list of conditions and the following disclaimers. 
*** Redistributions in binary form must reproduce the above copyright 
**notice, this list of conditions and the following disclaimers in the 
**documentation and/or other materials provided with the distribution. 
*** Neither the names of <Name of Development Group, Name of Institution>, 
**nor the names of its contributors may be used to endorse or promote 
**products derived from this Software without specific prior written 
**permission. 
**
**THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
**EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
**MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
**IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
**ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
**TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
**SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE. 
**========================================================== 
*/
/*
 * Copyright 2008 Benjamin C. Meyer <ben@meyerhome.net>
 * Copyright 2008 Jason A. Donenfeld <Jason@zx2c4.com>
 * Copyright 2008 Ariya Hidayat <ariya.hidayat@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/****************************************************************************
**
** Copyright (C) 2007-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "browsermainwindow.h"

#include "aboutdialog.h"
#include "autosaver.h"
#include "bookmarks.h"
#include "browserapplication.h"
#include "clearprivatedata.h"
#include "downloadmanager.h"
#include "history.h"
#include "Message.h"
#include "settings.h"
#include "tabbar.h"
#include "tabwidget.h"
#include "toolbarsearch.h"
#include "webview.h"
#include "webviewsearch.h"

#include <qdesktopwidget.h>
#include <qfiledialog.h>
#include <qplaintextedit.h>
#include <qprintdialog.h>
#include <qprintpreviewdialog.h>
#include <qprinter.h>
#include <qsettings.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qstatusbar.h>
#include <qtoolbar.h>
#include <qinputdialog.h>
#include <qsplitter.h>

#include <QSocketNotifier>
#include <QPainter>

#include <qdebug.h>

BrowserMainWindow::BrowserMainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , m_tabWidget(new TabWidget(this))
    , m_autoSaver(new AutoSaver(this))
    , m_historyBack(0)
    , m_historyForward(0)
    , m_stop(0)
    , m_reload(0)
{
    m_topMenu = NULL;
    setAttribute(Qt::WA_DeleteOnClose, true);
    statusBar()->setSizeGripEnabled(true);
    setupMenu();
    setupToolBar();

    QWidget *centralWidget = new QWidget(this);
    BookmarksModel *boomarksModel = BrowserApplication::bookmarksManager()->bookmarksModel();
    m_bookmarksToolbar = new BookmarksToolBar(boomarksModel, this);
    connect(m_bookmarksToolbar, SIGNAL(openUrl(const QUrl&, TabWidget::Tab, const QString&)),
            m_tabWidget, SLOT(loadUrl(const QUrl&, TabWidget::Tab, const QString&)));
    connect(m_bookmarksToolbar->toggleViewAction(), SIGNAL(toggled(bool)),
            this, SLOT(updateBookmarksToolbarActionText(bool)));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
#if defined(Q_WS_MAC)
    m_bookmarksToolbarFrame = new QFrame(this);
    m_bookmarksToolbarFrame->setLineWidth(1);
    m_bookmarksToolbarFrame->setMidLineWidth(0);
    m_bookmarksToolbarFrame->setFrameShape(QFrame::HLine);
    m_bookmarksToolbarFrame->setFrameShadow(QFrame::Raised);
    QPalette fp = m_bookmarksToolbarFrame->palette();
    fp.setColor(QPalette::Active, QPalette::Light, QColor(64, 64, 64));
    fp.setColor(QPalette::Active, QPalette::Dark, QColor(192, 192, 192));
    fp.setColor(QPalette::Inactive, QPalette::Light, QColor(135, 135, 135));
    fp.setColor(QPalette::Inactive, QPalette::Dark, QColor(226, 226, 226));
    m_bookmarksToolbarFrame->setAttribute(Qt::WA_MacNoClickThrough, true);
    m_bookmarksToolbarFrame->setPalette(fp);
    layout->addWidget(m_bookmarksToolbarFrame);

    layout->addWidget(m_bookmarksToolbar);
    QPalette p = m_bookmarksToolbar->palette();
    p.setColor(QPalette::Active, QPalette::Window, QColor(150, 150, 150));
    p.setColor(QPalette::Inactive, QPalette::Window, QColor(207, 207, 207));
    m_bookmarksToolbar->setAttribute(Qt::WA_MacNoClickThrough, true);
    m_bookmarksToolbar->setAutoFillBackground(true);
    m_bookmarksToolbar->setPalette(p);
    m_bookmarksToolbar->setBackgroundRole(QPalette::Window);
    m_bookmarksToolbar->setMaximumHeight(19);

    QWidget *w = new QWidget(this);
    w->setMaximumHeight(0);
    layout->addWidget(w); // <- OS X tab widget style bug
#else
    addToolBarBreak();
    
    addToolBar(m_bookmarksToolbar);
#endif
    layout->addWidget(m_tabWidget);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    connect(m_tabWidget, SIGNAL(loadPage(const QString &)),
            this, SLOT(loadPage(const QString &)));
    connect(m_tabWidget, SIGNAL(setCurrentTitle(const QString &)),
            this, SLOT(slotUpdateWindowTitle(const QString &)));
    connect(m_tabWidget, SIGNAL(showStatusBarMessage(const QString&)),
            statusBar(), SLOT(showMessage(const QString&)));
    connect(m_tabWidget, SIGNAL(linkHovered(const QString&)),
            statusBar(), SLOT(showMessage(const QString&)));
    connect(m_tabWidget, SIGNAL(loadProgress(int)),
            this, SLOT(slotLoadProgress(int)));
    connect(m_tabWidget, SIGNAL(tabsChanged()),
            m_autoSaver, SLOT(changeOccurred()));
    connect(m_tabWidget, SIGNAL(geometryChangeRequested(const QRect &)),
            this, SLOT(geometryChangeRequested(const QRect &)));
//    connect(m_tabWidget, SIGNAL(printRequested(QWebFrame *)),
//            this, SLOT(printRequested(QWebFrame *)));
    //connect(m_tabWidget, SIGNAL(menuBarVisibilityChangeRequested(bool)),
    //menuBar(), SLOT(setVisible(bool)));
    connect(m_tabWidget, SIGNAL(statusBarVisibilityChangeRequested(bool)),
            statusBar(), SLOT(setVisible(bool)));
    connect(m_tabWidget, SIGNAL(toolBarVisibilityChangeRequested(bool)),
            m_navigationBar, SLOT(setVisible(bool)));
    connect(m_tabWidget, SIGNAL(toolBarVisibilityChangeRequested(bool)),
            m_bookmarksToolbar, SLOT(setVisible(bool)));
    connect(m_tabWidget, SIGNAL(lastTabClosed()),
            this, SLOT(close()));

    slotUpdateWindowTitle();
    loadDefaultState();
    m_tabWidget->newTab();
    m_tabWidget->currentLineEdit()->setFocus();
    m_navigationBar->setIconSize(QSize(18, 18));

    // Add each item in the menu bar to the main window so
    // if the menu bar is hidden the shortcuts still work.
    /*
    QList<QAction *> actions = menuBar()->actions();
    foreach (QAction *action, actions) {
        if (action->menu())
            actions += action->menu()->actions();
        addAction(action);
    }
    */
#if defined(Q_WS_MAC)
    setWindowIcon(QIcon());
#endif

}

BrowserMainWindow::~BrowserMainWindow()
{
    m_autoSaver->changeOccurred();
    m_autoSaver->saveIfNeccessary();
}

void BrowserMainWindow::loadDefaultState()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("BrowserMainWindow"));
    QByteArray data = settings.value(QLatin1String("defaultState")).toByteArray();
    restoreState(data);
    settings.endGroup();
}

QSize BrowserMainWindow::sizeHint() const
{
    QRect desktopRect = QApplication::desktop()->screenGeometry();
    QSize size = desktopRect.size() * 0.9;
    return size;
}

void BrowserMainWindow::save()
{
    BrowserApplication::instance()->saveSession();

    QSettings settings;
    settings.beginGroup(QLatin1String("BrowserMainWindow"));
    QByteArray data = saveState(false);
    settings.setValue(QLatin1String("defaultState"), data);
    settings.endGroup();
}

static const qint32 BrowserMainWindowMagic = 0xba;

QByteArray BrowserMainWindow::saveState(bool withTabs) const
{
    int version = 2;
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << qint32(BrowserMainWindowMagic);
    stream << qint32(version);

    stream << size();
    stream << !m_navigationBar->isHidden();
    stream << !m_bookmarksToolbar->isHidden();
    stream << !statusBar()->isHidden();
    if (withTabs)
        stream << tabWidget()->saveState();
    else
        stream << QByteArray();
    stream << m_navigationSplitter->saveState();
    stream << m_tabWidget->tabBar()->showTabBarWhenOneTab();

    stream << qint32(toolBarArea(m_navigationBar));
    stream << qint32(toolBarArea(m_bookmarksToolbar));
    return data;
}

bool BrowserMainWindow::restoreState(const QByteArray &state)
{
    int version = 2;
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);
    if (stream.atEnd())
        return false;

    qint32 marker;
    qint32 v;
    stream >> marker;
    stream >> v;
    if (marker != BrowserMainWindowMagic || v != version)
        return false;

    QSize size;
    bool showToolbar;
    bool showBookmarksBar;
    bool showStatusbar;
    QByteArray tabState;
    QByteArray splitterState;
    bool showTabBarWhenOneTab;
    qint32 navigationBarLocation;
    qint32 bookmarkBarLocation;

    stream >> size;
    stream >> showToolbar;
    stream >> showBookmarksBar;
    stream >> showStatusbar;
    stream >> tabState;
    stream >> splitterState;
    stream >> showTabBarWhenOneTab;
    stream >> navigationBarLocation;
    stream >> bookmarkBarLocation;

    resize(size);

    m_navigationBar->setVisible(showToolbar);
    updateToolbarActionText(showToolbar);

    m_bookmarksToolbar->setVisible(showBookmarksBar);
#if defined(Q_WS_MAC)
    m_bookmarksToolbarFrame->setVisible(showBookmarksBar);
#endif
    updateBookmarksToolbarActionText(showBookmarksBar);

    statusBar()->setVisible(showStatusbar);
    updateStatusbarActionText(showStatusbar);

    m_navigationSplitter->restoreState(splitterState);

    if (!tabState.isEmpty() && !tabWidget()->restoreState(tabState))
        return false;

    m_tabWidget->tabBar()->setShowTabBarWhenOneTab(showTabBarWhenOneTab);

    Qt::ToolBarArea navigationArea = Qt::ToolBarArea(navigationBarLocation);
    if (navigationArea != Qt::TopToolBarArea && navigationArea != Qt::NoToolBarArea)
        addToolBar(navigationArea, m_navigationBar);
    Qt::ToolBarArea bookmarkArea = Qt::ToolBarArea(bookmarkBarLocation);
    if (bookmarkArea != Qt::TopToolBarArea && bookmarkArea != Qt::NoToolBarArea)
        addToolBar(bookmarkArea, m_bookmarksToolbar);

    return true;
}

QAction *BrowserMainWindow::showMenuBarAction() const
{
    return m_showMenuBarAction;
}

void BrowserMainWindow::setupMenu()
{
    new QShortcut(QKeySequence(Qt::Key_F6), this, SLOT(slotSwapFocus()));

    // File
    //m_topMenu = menuBar()->addMenu(tr("Menu"));
    m_topMenu = new QMenu(tr("Menu"), this);
    //QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QMenu *fileMenu = m_topMenu->addMenu(tr("&File"));

    fileMenu->addAction(tr("&New Window"), this, SLOT(slotFileNew()), QKeySequence::New);
    fileMenu->addAction(m_tabWidget->newTabAction());
    fileMenu->addAction(tr("&Open File..."), this, SLOT(slotFileOpen()), QKeySequence::Open);
    fileMenu->addAction(tr("Open &Location..."), this,
                        SLOT(slotSelectLineEdit()), QKeySequence(Qt::ControlModifier + Qt::Key_L));
    fileMenu->addSeparator();
    fileMenu->addAction(m_tabWidget->closeTabAction());
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Save As..."), this,
                        SLOT(slotFileSaveAs()), QKeySequence(QKeySequence::Save));
    fileMenu->addSeparator();
    BookmarksManager *bookmarksManager = BrowserApplication::bookmarksManager();
    fileMenu->addAction(tr("&Import Bookmarks..."), bookmarksManager, SLOT(importBookmarks()));
    fileMenu->addAction(tr("&Export Bookmarks..."), bookmarksManager, SLOT(exportBookmarks()));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("P&rint Preview..."), this, SLOT(slotFilePrintPreview()));
    fileMenu->addAction(tr("&Print..."), this, SLOT(slotFilePrint()), QKeySequence::Print);
    fileMenu->addSeparator();
    m_privateBrowsing = fileMenu->addAction(tr("Private &Browsing..."), this, SLOT(slotPrivateBrowsing()));
    m_privateBrowsing->setCheckable(true);
    fileMenu->addSeparator();

#if defined(Q_WS_MAC)
    fileMenu->addAction(tr("&Quit"), BrowserApplication::instance(), SLOT(quitBrowser()), QKeySequence(Qt::CTRL | Qt::Key_Q));
#else
    fileMenu->addAction(tr("&Quit"), this, SLOT(close()), QKeySequence(Qt::CTRL | Qt::Key_Q));
#endif

    // Edit
    //QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    QMenu *editMenu = m_topMenu->addMenu(tr("&Edit"));
    QAction *m_undo = editMenu->addAction(tr("&Undo"));
    m_undo->setShortcuts(QKeySequence::Undo);
    m_tabWidget->addWebAction(m_undo, WebView::Undo);
    QAction *m_redo = editMenu->addAction(tr("&Redo"));
    m_redo->setShortcuts(QKeySequence::Redo);
    m_tabWidget->addWebAction(m_redo, WebView::Redo);
    editMenu->addSeparator();
    QAction *m_cut = editMenu->addAction(tr("Cu&t"));
    m_cut->setShortcuts(QKeySequence::Cut);
    m_tabWidget->addWebAction(m_cut, WebView::Cut);
    QAction *m_copy = editMenu->addAction(tr("&Copy"));
    m_copy->setShortcuts(QKeySequence::Copy);
    m_tabWidget->addWebAction(m_copy, WebView::Copy);
    QAction *m_paste = editMenu->addAction(tr("&Paste"));
    m_paste->setShortcuts(QKeySequence::Paste);
    m_tabWidget->addWebAction(m_paste, WebView::Paste);
    editMenu->addSeparator();

    QAction *m_find = editMenu->addAction(tr("&Find"));
    m_find->setShortcuts(QKeySequence::Find);
    connect(m_find, SIGNAL(triggered()), this, SLOT(slotEditFind()));
    new QShortcut(QKeySequence(Qt::Key_Slash), this, SLOT(slotEditFind()));

    QAction *m_findNext = editMenu->addAction(tr("Find Nex&t"));
    m_findNext->setShortcuts(QKeySequence::FindNext);
    connect(m_findNext, SIGNAL(triggered()), this, SLOT(slotEditFindNext()));

    QAction *m_findPrevious = editMenu->addAction(tr("Find P&revious"));
    m_findPrevious->setShortcuts(QKeySequence::FindPrevious);
    connect(m_findPrevious, SIGNAL(triggered()), this, SLOT(slotEditFindPrevious()));

    editMenu->addSeparator();
    editMenu->addAction(tr("Prefere&nces..."), this, SLOT(slotPreferences()), tr("Ctrl+,"));

    // View
    //QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    QMenu *viewMenu = m_topMenu->addMenu(tr("&View"));
    m_showMenuBarAction = new QAction(tr("Show Menu Bar"), this);
    m_showMenuBarAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
    //connect(m_showMenuBarAction, SIGNAL(triggered()), this, SLOT(slotViewMenuBar()));
    addAction(m_showMenuBarAction);


    m_viewToolbar = new QAction(this);
    updateToolbarActionText(true);
    m_viewToolbar->setShortcut(tr("Ctrl+|"));
    connect(m_viewToolbar, SIGNAL(triggered()), this, SLOT(slotViewToolbar()));
    viewMenu->addAction(m_viewToolbar);

    m_viewBookmarkBar = new QAction(this);
    updateBookmarksToolbarActionText(true);
    m_viewBookmarkBar->setShortcut(tr("Shift+Ctrl+B"));
    connect(m_viewBookmarkBar, SIGNAL(triggered()), this, SLOT(slotViewBookmarksBar()));
    viewMenu->addAction(m_viewBookmarkBar);

    QAction *viewTabBarAction = m_tabWidget->tabBar()->viewTabBarAction();
    viewMenu->addAction(viewTabBarAction);
    connect(viewTabBarAction, SIGNAL(changed()),
            m_autoSaver, SLOT(changeOccurred()));

    m_viewStatusbar = new QAction(this);
    updateStatusbarActionText(true);
    m_viewStatusbar->setShortcut(tr("Ctrl+/"));
    connect(m_viewStatusbar, SIGNAL(triggered()), this, SLOT(slotViewStatusbar()));
    viewMenu->addAction(m_viewStatusbar);

    viewMenu->addSeparator();

    m_stop = viewMenu->addAction(tr("&Stop"));
    QList<QKeySequence> shortcuts;
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Period));
    shortcuts.append(Qt::Key_Escape);
    m_stop->setShortcuts(shortcuts);
    m_tabWidget->addWebAction(m_stop, WebView::Stop);

    m_reload = viewMenu->addAction(tr("&Reload Page"));
    m_reload->setShortcuts(QKeySequence::Refresh);
    m_tabWidget->addWebAction(m_reload, WebView::Reload);

    viewMenu->addAction(tr("Make Text &Bigger"), this, SLOT(slotViewTextBigger()), QKeySequence(Qt::CTRL | Qt::Key_Plus));
    viewMenu->addAction(tr("Make Text &Normal"), this, SLOT(slotViewTextNormal()), QKeySequence(Qt::CTRL | Qt::Key_0));
    viewMenu->addAction(tr("Make Text &Smaller"), this, SLOT(slotViewTextSmaller()), QKeySequence(Qt::CTRL | Qt::Key_Minus));

    viewMenu->addSeparator();
    viewMenu->addAction(tr("Page S&ource"), this, SLOT(slotViewPageSource()), tr("Ctrl+Alt+U"));
    QAction *a = viewMenu->addAction(tr("&Full Screen"), this, SLOT(slotViewFullScreen(bool)),  Qt::Key_F11);
    a->setCheckable(true);

    // History
    HistoryMenu *historyMenu = new HistoryMenu(this);
    connect(historyMenu, SIGNAL(openUrl(const QUrl&)),
            m_tabWidget, SLOT(loadUrl(const QUrl&)));
    historyMenu->setTitle(tr("Hi&story"));
    //menuBar()->addMenu(historyMenu);
    m_topMenu->addMenu(historyMenu);
    QList<QAction*> historyActions;

    m_historyBack = new QAction(tr("Back"), this);
    m_tabWidget->addWebAction(m_historyBack, WebView::Back);
    m_historyBack->setShortcuts(QKeySequence::Back);
    m_historyBack->setIconVisibleInMenu(false);

    m_historyForward = new QAction(tr("Forward"), this);
    m_tabWidget->addWebAction(m_historyForward, WebView::Forward);
    m_historyForward->setShortcuts(QKeySequence::Forward);
    m_historyForward->setIconVisibleInMenu(false);

    QAction *m_historyHome = new QAction(tr("Home"), this);
    connect(m_historyHome, SIGNAL(triggered()), this, SLOT(slotHome()));
    m_historyHome->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H));

    m_restoreLastSession = new QAction(tr("Restore Last Session"), this);
    connect(m_restoreLastSession, SIGNAL(triggered()), BrowserApplication::instance(), SLOT(restoreLastSession()));
    m_restoreLastSession->setEnabled(BrowserApplication::instance()->canRestoreSession());

    historyActions.append(m_historyBack);
    historyActions.append(m_historyForward);
    historyActions.append(m_historyHome);
    historyActions.append(m_tabWidget->recentlyClosedTabsAction());
    historyActions.append(m_restoreLastSession);
    historyMenu->setInitialActions(historyActions);

    // Bookmarks
    BookmarksMenu *bookmarksMenu = new BookmarksMenu(this);
    connect(bookmarksMenu, SIGNAL(openUrl(const QUrl&, TabWidget::Tab, const QString &)),
            m_tabWidget, SLOT(loadUrl(const QUrl&, TabWidget::Tab, const QString&)));
    bookmarksMenu->setTitle(tr("&Bookmarks"));
    //menuBar()->addMenu(bookmarksMenu);
    m_topMenu->addMenu(bookmarksMenu);

    QList<QAction*> bookmarksActions;

    QAction *showAllBookmarksAction = new QAction(tr("Manage Bookmarks..."), this);
    showAllBookmarksAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B));
    connect(showAllBookmarksAction, SIGNAL(triggered()), this, SLOT(slotShowBookmarksDialog()));
    m_addBookmark = new QAction(QIcon(QLatin1String(":addbookmark.png")), tr("Add Bookmark..."), this);
    m_addBookmark->setIconVisibleInMenu(false);

    connect(m_addBookmark, SIGNAL(triggered()), this, SLOT(slotAddBookmark()));
    m_addBookmark->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

    bookmarksActions.append(showAllBookmarksAction);
    bookmarksActions.append(m_addBookmark);
    bookmarksMenu->setInitialActions(bookmarksActions);

    // Window
    //m_windowMenu = menuBar()->addMenu(tr("&Window"));
    m_windowMenu = m_topMenu->addMenu(tr("&Window"));
    connect(m_windowMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotAboutToShowWindowMenu()));
    slotAboutToShowWindowMenu();

    //QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    QMenu *toolsMenu = m_topMenu->addMenu(tr("&Tools"));
    toolsMenu->addAction(tr("Web &Search"), this, SLOT(slotWebSearch()),
                         QKeySequence(tr("Ctrl+K", "Web Search")));
    toolsMenu->addAction(tr("&Clear Private Data"), this, SLOT(slotClearPrivateData()),
                         QKeySequence(tr("Ctrl+Shift+Delete", "Clear Private Data")));
    toolsMenu->addAction(tr("Snapshot Window"), this, SLOT(slotSnapShot()));
#ifndef Q_CC_MINGW
    QAction *m_enableInspector = toolsMenu->addAction(tr("Enable Web &Inspector"), this, SLOT(slotToggleInspector(bool)));
    m_enableInspector->setCheckable(true);
    QSettings settings;
    settings.beginGroup(QLatin1String("websettings"));
    m_enableInspector->setChecked(settings.value(QLatin1String("enableInspector"), false).toBool());
#endif

    // Help
    //QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QMenu *helpMenu = m_topMenu->addMenu(tr("&Help"));
    helpMenu->addAction(tr("About &Qt"), qApp, SLOT(aboutQt()));
    helpMenu->addAction(tr("About &OP Browser"), this, SLOT(slotAboutApplication()));
}

void BrowserMainWindow::setupToolBar()
{
    setUnifiedTitleAndToolBarOnMac(true);
    m_navigationBar = addToolBar(tr("Navigation"));
    connect(m_navigationBar->toggleViewAction(), SIGNAL(toggled(bool)),
            this, SLOT(updateToolbarActionText(bool)));

    m_historyBack->setIcon(style()->standardIcon(QStyle::SP_ArrowBack, 0, this));
    m_historyBackMenu = new QMenu(this);
    m_historyBack->setMenu(m_historyBackMenu);
    connect(m_historyBackMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotAboutToShowBackMenu()));
    connect(m_historyBackMenu, SIGNAL(triggered(QAction *)),
            this, SLOT(slotOpenActionUrl(QAction *)));
    m_navigationBar->addAction(m_historyBack);

    m_historyForward->setIcon(style()->standardIcon(QStyle::SP_ArrowForward, 0, this));
    m_historyForwardMenu = new QMenu(this);
    connect(m_historyForwardMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotAboutToShowForwardMenu()));
    connect(m_historyForwardMenu, SIGNAL(triggered(QAction *)),
            this, SLOT(slotOpenActionUrl(QAction *)));
    m_historyForward->setMenu(m_historyForwardMenu);
    m_navigationBar->addAction(m_historyForward);

    m_stopReload = new QAction(this);
    m_reloadIcon = style()->standardIcon(QStyle::SP_BrowserReload);
    m_stopReload->setIcon(m_reloadIcon);

    m_navigationBar->addAction(m_stopReload);

    m_navigationSplitter = new QSplitter(m_navigationBar);
    m_navigationSplitter->addWidget(m_tabWidget->lineEditStack());
    m_toolbarSearch = new ToolbarSearch(m_navigationBar);
    m_navigationSplitter->addWidget(m_toolbarSearch);
    connect(m_toolbarSearch, SIGNAL(search(const QUrl&)),
            m_tabWidget, SLOT(loadUrl(const QUrl&)));
    m_navigationSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_tabWidget->lineEditStack()->setMinimumWidth(120);
    m_navigationSplitter->setCollapsible(0, false);
    m_navigationBar->addWidget(m_navigationSplitter);
    int splitterWidth = m_navigationSplitter->width();
    QList<int> sizes;
    sizes << (int)((double)splitterWidth * .80) << (int)((double)splitterWidth * .20);
    m_navigationSplitter->setSizes(sizes);

    m_topMenuAction = new QAction(tr("Menu"), this);
    m_topMenuAction->setIconVisibleInMenu(false);
    m_topMenuAction->setMenu(m_topMenu);
    m_navigationBar->addAction(m_topMenuAction);

}

void BrowserMainWindow::slotShowBookmarksDialog()
{
    BookmarksDialog *dialog = new BookmarksDialog(this);
    connect(dialog, SIGNAL(openUrl(const QUrl&, TabWidget::Tab, const QString &)),
            m_tabWidget, SLOT(loadUrl(const QUrl&, TabWidget::Tab, const QString &)));
    dialog->show();
}

void BrowserMainWindow::slotAddBookmark()
{
    WebView *webView = currentTab();
    QString url = webView->url().toString();
    QString title = webView->title();
    AddBookmarkDialog dialog(url, title);
    dialog.exec();
}

/*
void BrowserMainWindow::slotViewMenuBar()
{
    menuBar()->setVisible(!menuBar()->isVisible());
}
*/

void BrowserMainWindow::slotViewToolbar()
{
    if (m_navigationBar->isVisible()) {
        updateToolbarActionText(false);
        m_navigationBar->close();
    } else {
        updateToolbarActionText(true);
        m_navigationBar->show();
    }
    m_autoSaver->changeOccurred();
}

void BrowserMainWindow::slotViewBookmarksBar()
{
    if (m_bookmarksToolbar->isVisible()) {
        updateBookmarksToolbarActionText(false);
        m_bookmarksToolbar->hide();
#if defined(Q_WS_MAC)
        m_bookmarksToolbarFrame->hide();
#endif
    } else {
        updateBookmarksToolbarActionText(true);
        m_bookmarksToolbar->show();
#if defined(Q_WS_MAC)
        m_bookmarksToolbarFrame->show();
#endif
    }
    m_autoSaver->changeOccurred();
}

void BrowserMainWindow::updateStatusbarActionText(bool visible)
{
    m_viewStatusbar->setText(!visible ? tr("Show Status Bar") : tr("Hide Status Bar"));
}

void BrowserMainWindow::updateToolbarActionText(bool visible)
{
    m_viewToolbar->setText(!visible ? tr("Show Toolbar") : tr("Hide Toolbar"));
}

void BrowserMainWindow::updateBookmarksToolbarActionText(bool visible)
{
    m_viewBookmarkBar->setText(!visible ? tr("Show Bookmarks Bar") : tr("Hide Bookmarks Bar"));
}

void BrowserMainWindow::slotViewStatusbar()
{
    if (statusBar()->isVisible()) {
        updateStatusbarActionText(false);
        statusBar()->close();
    } else {
        updateStatusbarActionText(true);
        statusBar()->show();
    }
    m_autoSaver->changeOccurred();
}

QUrl BrowserMainWindow::guessUrlFromString(const QString &string)
{
    QString urlStr = string.trimmed();

    // check if the string is just a host with a port
    QRegExp hostWithPort(QLatin1String("^[a-zA-Z\\.]+\\:[0-9]*$"));
    if (hostWithPort.exactMatch(urlStr))
        urlStr = QLatin1String("http://") + urlStr;

    // Check if it looks like a qualified URL. Try parsing it and see.
    QRegExp test(QLatin1String("^[a-zA-Z]+\\:.*"));
    bool hasSchema = test.exactMatch(urlStr);
    if (hasSchema) {
        bool isAscii = true;
        foreach (QChar c, urlStr) {
            if (c >= 0x80) {
                isAscii = false;
                break;
            }
        }

        QUrl url;
        if (isAscii) {
            url = QUrl::fromEncoded(urlStr.toAscii(), QUrl::TolerantMode);
        } else {
            url = QUrl(urlStr, QUrl::TolerantMode);
        }
        if (url.isValid())
            return url;
    }

    // Might be a file.
    if (QFile::exists(urlStr)) {
        QFileInfo info(urlStr);
        return QUrl::fromLocalFile(info.absoluteFilePath());
    }

    // Might be a shorturl - try to detect the schema.
    if (!hasSchema) {
        int dotIndex = urlStr.indexOf(QLatin1Char('.'));
        if (dotIndex != -1) {
            QString prefix = urlStr.left(dotIndex).toLower();
            QString schema = (prefix == QLatin1String("ftp")) ? prefix : QLatin1String("http");
            QUrl url(schema + QLatin1String("://") + urlStr, QUrl::TolerantMode);
            if (url.isValid())
                return url;
        }
    }

    // Fall back to QUrl's own tolerant parser.
    QUrl url = QUrl(string, QUrl::TolerantMode);

    // finally for cases where the user just types in a hostname add http
    if (url.scheme().isEmpty())
        url = QUrl(QLatin1String("http://") + string, QUrl::TolerantMode);
    return url;
}

void BrowserMainWindow::slotDownloadManager()
{
    BrowserApplication::downloadManager()->show();
}

void BrowserMainWindow::slotSelectLineEdit()
{
    m_tabWidget->currentLineEdit()->selectAll();
    m_tabWidget->currentLineEdit()->setFocus();
}

void BrowserMainWindow::slotFileSaveAs()
{
    notImpl();
    //    BrowserApplication::downloadManager()->download(currentTab()->url(), true);
}

void BrowserMainWindow::slotPreferences()
{
    SettingsDialog settingsDialog(this);
    settingsDialog.exec();
}

void BrowserMainWindow::slotUpdateStatusbar(const QString &string)
{
    statusBar()->showMessage(string, 2000);
}

void BrowserMainWindow::slotUpdateWindowTitle(const QString &title)
{
    if (title.isEmpty()) {
        setWindowTitle(tr("Opus Palladianum"));
    } else {
#if defined(Q_WS_MAC)
        setWindowTitle(title);
#else
        setWindowTitle(tr("%1 - Opus Palladianum").arg(title));
#endif
    }
}

void BrowserMainWindow::slotAboutApplication()
{
    AboutDialog *aboutDialog = new AboutDialog(this);
    aboutDialog->show();
}

void BrowserMainWindow::slotFileNew()
{
    BrowserApplication::instance()->newMainWindow();
    BrowserMainWindow *mw = BrowserApplication::instance()->mainWindow();
    mw->slotHome();
}

void BrowserMainWindow::slotFileOpen()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Open Web Resource"), QString(),
                   tr("Web Resources (*.html *.htm *.svg *.png *.gif *.svgz);;All files (*.*)"));

    if (file.isEmpty())
        return;

    loadPage(file);
}

void BrowserMainWindow::slotFilePrintPreview()
{
    if (!currentTab())
        return;
    QPrintPreviewDialog *dialog = new QPrintPreviewDialog(this);
    connect(dialog, SIGNAL(paintRequested(QPrinter *)),
            currentTab(), SLOT(print(QPrinter *)));
    dialog->exec();
}

void BrowserMainWindow::slotFilePrint()
{
    if (!currentTab())
        return;
    printRequested(currentTab());
}

void BrowserMainWindow::printRequested(WebView* view)
{
    QPrinter printer;
    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    dialog->setWindowTitle(tr("Print Document"));
    if (dialog->exec() != QDialog::Accepted)
        return;
    view->print();
}

void BrowserMainWindow::slotPrivateBrowsing()
{
    std::cerr << "Not Impl" << std::endl;
/*    QWebSettings *settings = QWebSettings::globalSettings();
    bool pb = settings->testAttribute(QWebSettings::PrivateBrowsingEnabled);
    if (!pb) {
        QString title = tr("Are you sure you want to turn on private browsing?");
        QString text = tr("<b>%1</b><br><br>When private browsing is turned on,"
            " some actions concerning your privacy will be disabled:"
            "<ul><li> Webpages are not added to the history.</li>"
            "<li> Items are automatically removed from the Downloads window.</li>"
            "<li> New cookies are not stored, current cookies can't be accessed.</li>"
            "<li> Site icons won't be stored, session won't be saved.</li>"
            "<li> Searches are not addded to the pop-up menu in the search box.</li></ul>"
            "Until you close the window, you can still click the Back and Forward buttons"
            " to return to the webpages you have opened.").arg(title);

        QMessageBox::StandardButton button = QMessageBox::question(this, QString(), text,
                               QMessageBox::Ok | QMessageBox::Cancel,
                               QMessageBox::Ok);
        if (button == QMessageBox::Ok) {
            settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);
        }
    } else {
        settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, false);

        QList<BrowserMainWindow*> windows = BrowserApplication::instance()->mainWindows();
        for (int i = 0; i < windows.count(); ++i) {
            BrowserMainWindow *window = windows.at(i);
            window->tabWidget()->clear();
        }
    }
    m_privateBrowsing->setChecked(settings->testAttribute(QWebSettings::PrivateBrowsingEnabled));*/
}

void BrowserMainWindow::closeEvent(QCloseEvent *event)
{
    if (m_tabWidget->count() > 1) {
        QSettings settings;
        settings.beginGroup(QLatin1String("tabs"));
        bool confirm = settings.value(QLatin1String("confirmClosingMultipleTabs"), true).toBool();
        if (confirm) {
            int ret = QMessageBox::warning(this, QString(),
                                           tr("Are you sure you want to close the window?"
                                              "  There are %1 tabs open").arg(m_tabWidget->count()),
                                           QMessageBox::Yes | QMessageBox::No,
                                           QMessageBox::No);
            if (ret == QMessageBox::No) {
                event->ignore();
                return;
            }
        }
    }
    event->accept();
    deleteLater();
}

void BrowserMainWindow::mousePressEvent(QMouseEvent *event)
{
	switch(event->button())
	{
	case Qt::XButton1:
		m_historyBack->activate(QAction::Trigger);
		break;
	case Qt::XButton2:
		m_historyForward->activate(QAction::Trigger);
		break;
	default:
		QMainWindow::mousePressEvent(event);
		break;
	}
}

void BrowserMainWindow::slotEditFind()
{
    tabWidget()->webViewSearch(m_tabWidget->currentIndex())->showFind();
}

void BrowserMainWindow::slotEditFindNext()
{
    tabWidget()->webViewSearch(m_tabWidget->currentIndex())->findNext();
}

void BrowserMainWindow::slotEditFindPrevious()
{
    tabWidget()->webViewSearch(m_tabWidget->currentIndex())->findPrevious();
}

void BrowserMainWindow::slotViewTextBigger()
{
    if (!currentTab())
        return;
    currentTab()->zoomIn();
}

void BrowserMainWindow::slotViewTextNormal()
{
    if (!currentTab())
        return;
    currentTab()->resetZoom();
}

void BrowserMainWindow::slotViewTextSmaller()
{
    if (!currentTab())
        return;
    currentTab()->zoomOut();
}

void BrowserMainWindow::slotViewFullScreen(bool makeFullScreen)
{
    if (makeFullScreen) {
        setWindowState(windowState() | Qt::WindowFullScreen);
    } else {
        setWindowState(windowState() & ~Qt::WindowFullScreen);
    }
}

void BrowserMainWindow::slotViewPageSource()
{
    if (!currentTab())
        return;

    std::cerr << "Not Impl" << std::endl;
    /*QString markup = currentTab()->page()->mainFrame()->toHtml();
    QPlainTextEdit *view = new QPlainTextEdit(markup);
    view->setWindowTitle(tr("Page Source of %1").arg(currentTab()->title()));
    view->setMinimumWidth(640);
    view->setAttribute(Qt::WA_DeleteOnClose);
    view->show();*/
}

void BrowserMainWindow::slotHome()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("MainWindow"));
    QString home = settings.value(QLatin1String("home"), QLatin1String("http://www.cs.uiuc.edu/homes/stang6/")).toString();
    loadPage(home);
}

void BrowserMainWindow::slotWebSearch()
{
    m_toolbarSearch->selectAll();
    m_toolbarSearch->setFocus();
}

void BrowserMainWindow::slotClearPrivateData()
{
    ClearPrivateData dialog;
    dialog.exec();
}

void BrowserMainWindow::slotSnapShot()
{
    WebView *view = currentTab();
    view->sendWebAppMsg(MSG_SNAP_SHOT, tr("/tmp/snapshot.PNG").toAscii());
}

void BrowserMainWindow::slotToggleInspector(bool enable)
{
    std::cerr << "Not Impl" << std::endl;
    /*
    QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, enable);
    if (enable) {
        int result = QMessageBox::question(this, tr("Web Inspector"),
                                           tr("The web inspector will only work correctly for pages that were loaded after enabling.\n"
                                              "Do you want to reload all pages?"),
                                           QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::Yes) {
            m_tabWidget->reloadAllTabs();
        }
        } */
    QSettings settings;
    settings.beginGroup(QLatin1String("websettings"));
    settings.setValue(QLatin1String("enableInspector"), enable);
}

void BrowserMainWindow::slotSwapFocus()
{
    if (currentTab()->hasFocus()) {
        m_tabWidget->currentLineEdit()->setFocus();
        m_tabWidget->currentLineEdit()->selectAll();
    } else {
        currentTab()->setFocus();
    }
}

void BrowserMainWindow::loadPage(const QString &page)
{
    if (!currentTab() || page.isEmpty())
        return;

    QUrl url = guessUrlFromString(page);
    m_tabWidget->currentLineEdit()->setText(url.toString());
    m_tabWidget->loadUrl(url, TabWidget::CurrentTab);
}

TabWidget *BrowserMainWindow::tabWidget() const
{
    return m_tabWidget;
}

WebView *BrowserMainWindow::currentTab() const
{
    return m_tabWidget->currentWebView();
}

ToolbarSearch *BrowserMainWindow::toolbarSearch() const
{
    return m_toolbarSearch;
}

void BrowserMainWindow::slotLoadProgress(int progress)
{
    if (progress < 100 && progress > 0) {
        disconnect(m_stopReload, SIGNAL(triggered()), m_reload, SLOT(trigger()));
        if (m_stopIcon.isNull())
            m_stopIcon = style()->standardIcon(QStyle::SP_BrowserStop);
        m_stopReload->setIcon(m_stopIcon);
        connect(m_stopReload, SIGNAL(triggered()), m_stop, SLOT(trigger()));
        m_stopReload->setToolTip(tr("Stop loading the current page"));
    } else {
        disconnect(m_stopReload, SIGNAL(triggered()), m_stop, SLOT(trigger()));
        m_stopReload->setIcon(m_reloadIcon);
        connect(m_stopReload, SIGNAL(triggered()), m_reload, SLOT(trigger()));
        m_stopReload->setToolTip(tr("Reload the current page"));
    }
}

void BrowserMainWindow::slotAboutToShowBackMenu()
{
    m_historyBackMenu->clear();
    if (!currentTab())
        return;
    History *history = currentTab()->history();
    int historyCount = history->count();
    for (int i = history->backItems(historyCount).count() - 1; i >= 0; --i) {
        HistoryItem item = history->backItems(history->count()).at(i);
        QAction *action = new QAction(this);
        action->setData(-1*(historyCount - i - 1));
        QIcon icon = BrowserApplication::instance()->icon(item.url);
        action->setIcon(icon);
        if (!item.title.isEmpty())
            action->setText(item.title);
        else
            action->setText(item.url);
        m_historyBackMenu->addAction(action);
    }
}

void BrowserMainWindow::slotAboutToShowForwardMenu()
{
    m_historyForwardMenu->clear();
    if (!currentTab())
        return;
    History *history = currentTab()->history();
    int historyCount = history->count();
    for (int i = 0; i < history->forwardItems(history->count()).count(); ++i) {
        HistoryItem item = history->forwardItems(historyCount).at(i);
        QAction *action = new QAction(this);
        action->setData(historyCount - i);
        QIcon icon = BrowserApplication::instance()->icon(item.url);
        action->setIcon(icon);
        if (!item.title.isEmpty())
            action->setText(item.title);
        else
            action->setText(item.url);
        m_historyForwardMenu->addAction(action);
    }
}

void BrowserMainWindow::slotAboutToShowWindowMenu()
{
    m_windowMenu->clear();
    m_windowMenu->addAction(m_tabWidget->nextTabAction());
    m_windowMenu->addAction(m_tabWidget->previousTabAction());
    m_windowMenu->addSeparator();
    m_windowMenu->addAction(tr("Downloads"), this, SLOT(slotDownloadManager()), QKeySequence(tr("Alt+Ctrl+L", "Download Manager")));

    m_windowMenu->addSeparator();
    QList<BrowserMainWindow*> windows = BrowserApplication::instance()->mainWindows();
    for (int i = 0; i < windows.count(); ++i) {
        BrowserMainWindow *window = windows.at(i);
        QAction *action = m_windowMenu->addAction(window->windowTitle(), this, SLOT(slotShowWindow()));
        action->setData(i);
        action->setCheckable(true);
        if (window == this)
            action->setChecked(true);
    }
}

void BrowserMainWindow::slotShowWindow()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        QVariant v = action->data();
        if (v.canConvert<int>()) {
            int offset = qvariant_cast<int>(v);
            QList<BrowserMainWindow*> windows = BrowserApplication::instance()->mainWindows();
            windows.at(offset)->activateWindow();
            windows.at(offset)->currentTab()->setFocus();
        }
    }
}

void BrowserMainWindow::slotOpenActionUrl(QAction *action)
{
    int offset = action->data().toInt();
    History *history = currentTab()->history();
    if (offset < 0)
        history->goToItem(history->backItems(-1*offset).first()); // back
    else if (offset > 0)
        history->goToItem(history->forwardItems(history->count() - offset + 1).back()); // forward
}

void BrowserMainWindow::geometryChangeRequested(const QRect &geometry)
{
    setGeometry(geometry);
}

