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
**¡°Software¡±), to deal with the Software without restriction, including 
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

#include "browserapplication.h"

#include "bookmarks.h"
#include "browsermainwindow.h"
#include "downloadmanager.h"
#include "history.h"
#include "tabwidget.h"
#include "webview.h"

#include "Message.h"

#include <qbuffer.h>
//#include <qdesktopservices.h>
#include <qdir.h>
#include <qevent.h>
#include <qlibraryinfo.h>
#include <qmessagebox.h>
#include <qsettings.h>
#include <qtextstream.h>
#include <qtranslator.h>


#include <qdebug.h>
#include <iostream>
#include <QSocketNotifier>


void notImpl() { std::cerr << "notImpl" << std::endl; }

DownloadManager *BrowserApplication::s_downloadManager = 0;
HistoryManager *BrowserApplication::s_historyManager = 0;
BookmarksManager *BrowserApplication::s_bookmarksManager = 0;

bool BrowserApplication::isTheOnlyBrowser() const {return true;}

BrowserApplication::BrowserApplication(int &argc, char **argv)
    : QApplication(argc, argv)
    , quiting(false)
    , m_stdin(0)
{
    QCoreApplication::setOrganizationDomain(QLatin1String("cs@illinois"));
    QCoreApplication::setApplicationName(QLatin1String("OP Browser"));
    QString version = QLatin1String("beta");

    QCoreApplication::setApplicationVersion(version);

#if defined(Q_WS_MAC)
    QApplication::setQuitOnLastWindowClosed(false);
#else
    QApplication::setQuitOnLastWindowClosed(true);
#endif

    QString localSysName = QLocale::system().name();
    installTranslator(QLatin1String("qt_") + localSysName);
    installTranslator(dataDirectory() + QDir::separator() + QLatin1String("locale") + QDir::separator() + localSysName);

    QSettings settings;
    settings.beginGroup(QLatin1String("sessions"));
    m_lastSession = settings.value(QLatin1String("lastSession")).toByteArray();
    settings.endGroup();

#if defined(Q_WS_MAC)
    connect(this, SIGNAL(lastWindowClosed()),
            this, SLOT(lastWindowClosed()));
#endif

    QTimer::singleShot(0, this, SLOT(postLaunch()));

    m_stdin = new QSocketNotifier(Message::getReadFd(), QSocketNotifier::Read, this);
    connect(m_stdin, SIGNAL(activated(int)), this, SLOT(dispatch(int)));

}

BrowserApplication::~BrowserApplication()
{
    quiting = true;
    delete s_downloadManager;
    qDeleteAll(m_mainWindows);
    delete s_bookmarksManager;
}

void BrowserApplication::dispatch(int socket) {

    if (socket == Message::getReadFd()) {
        clean();
        m_stdin->setEnabled(false);
        Message *msg = new Message;
        
        msg->readMessage();

        WebView* destView = 0;
        i32 clientId = 0;
        i32 containerId = 0;
        if (msg->getMsgType() == MSG_NEW_WEBAPP) {
            containerId = atoi(msg->getStringData().c_str());
            
        } 
        clientId = msg->getSrcId();
        
        if (m_mainWindows[0]->currentTab() &&
            (m_mainWindows[0]->currentTab()->clientId() == clientId
             || m_mainWindows[0]->currentTab()->ownId() == containerId)) {
            destView = m_mainWindows[0]->currentTab();
        } else {
            for (int winIdx = 0; winIdx < m_mainWindows.size() && destView == 0; ++winIdx) {
                TabWidget* tabs = m_mainWindows[winIdx]->tabWidget();
                for (int tabIdx = 0; tabIdx < tabs->count() && destView == 0; ++tabIdx) {
                    WebView* view = tabs->webView(tabIdx);
                    if (view != 0) {
                        if (view->clientId() == clientId || view->ownId() == containerId) {
                            destView = view;
                        }
                    }
                }
            }   
        }
        
        if (msg->getMsgType() == MSG_NEW_WEBAPP) {
            if (destView)
                destView->handleEmbed(msg);
            else {
                if (m_mainWindows[0]->currentTab() &&
                    m_mainWindows[0]->currentTab()->clientId() == 0) {
                    m_mainWindows[0]->currentTab()->handleEmbed(msg);
                } else {
                    if (containerId == 2)
                        m_mainWindows[0]->tabWidget()->makeNewTab()->handleEmbed(msg);
                    if (containerId == 3)
                        m_mainWindows[0]->tabWidget()->makeNewTab(true)->handleEmbed(msg);
                    if (containerId == 4)
                        newMainWindow()->currentTab()->handleEmbed(msg);
                    if (containerId == 5)
                        m_mainWindows[0]->currentTab()->prepFrame(msg);
                    if (containerId > 5) {
                        // TODO, find a way to identify the case:
                        // iframe has a new instance to embed
                        m_mainWindows[0]->currentTab()->embedFrame(msg);
                    }
                }
            }
        } else if (msg->getMsgType() == MSG_NAV_SET_WEBAPP) {
            if (destView)
                destView->navToNewUrl(QString::fromAscii((const char*)msg->getMsgData()));
        } else if (msg->getMsgType() == MSG_EMBED_FRAME) {
            m_mainWindows[0]->currentTab()->embedFrame(msg);
        } else if (msg->getMsgType() == MSG_UI_MSG){
            if (destView)
                destView->dispatchUIMsg(msg);
        } else if (msg->getMsgType() == MSG_DOWNLOAD_INFO) {
            downloadManager()->handleDownloadInfo(msg->getMsgValue(), QByteArray((const char*)(msg->getMsgData()), msg->getDataLen()));
        } else {
            qDebug() << "Unhandled UI message type:" << msg->getMsgType();
        }
            

        delete msg;
        m_stdin->setEnabled(true);
    }
}


#if defined(Q_WS_MAC)
void BrowserApplication::lastWindowClosed()
{
    clean();
    BrowserMainWindow *mw = new BrowserMainWindow;
    mw->slotHome();
    m_mainWindows.prepend(mw);
}
#endif

BrowserApplication *BrowserApplication::instance()
{
    return (static_cast<BrowserApplication *>(QCoreApplication::instance()));
}

#if defined(Q_WS_MAC)
#include <qmessagebox.h>
void BrowserApplication::quitBrowser()
{
    clean();
    int tabCount = 0;
    for (int i = 0; i < m_mainWindows.count(); ++i) {
        tabCount += m_mainWindows.at(i)->tabWidget()->count();
    }

    if (tabCount > 1) {
        int ret = QMessageBox::warning(mainWindow(), QString(),
                           tr("There are %1 windows and %2 tabs open\n"
                              "Do you want to quit anyway?").arg(m_mainWindows.count()).arg(tabCount),
                           QMessageBox::Yes | QMessageBox::No,
                           QMessageBox::No);
        if (ret == QMessageBox::No)
            return;
    }

    saveSession();
    exit(0);
}
#endif

/*!
    Any actions that can be delayed until the window is visible
 */
void BrowserApplication::postLaunch()
{

    setWindowIcon(QIcon(QLatin1String(":128x128/arora.png")));

    loadSettings();

    // newMainWindow() needs to be called in main() for this to happen
    if (m_mainWindows.count() > 0) {
        QSettings settings;
        settings.beginGroup(QLatin1String("MainWindow"));
        int startup = settings.value(QLatin1String("startupBehavior")).toInt();
        QStringList args = QCoreApplication::arguments();

        if (args.count() > 1) {
            switch (startup) {
            case 2: {
                restoreLastSession();
                WebView *webView = mainWindow()->tabWidget()->makeNewTab(true);
                webView->loadUrl(args.last());
                break;
            }
            default:
                mainWindow()->loadPage(args.last());
                break;
            }
        } else {
            switch (startup) {
            case 0:
                mainWindow()->slotHome();
                break;
            case 1:
                break;
            case 2:
                restoreLastSession();
                break;
            }
        }
    }
    BrowserApplication::historyManager();
}

void BrowserApplication::loadSettings()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("websettings"));

    /*QWebSettings *defaultSettings = QWebSettings::globalSettings();
    QString standardFontFamily = defaultSettings->fontFamily(QWebSettings::StandardFont);
    int standardFontSize = defaultSettings->fontSize(QWebSettings::DefaultFontSize);
    QFont standardFont = QFont(standardFontFamily, standardFontSize);
    standardFont = qVariantValue<QFont>(settings.value(QLatin1String("standardFont"), standardFont));
    defaultSettings->setFontFamily(QWebSettings::StandardFont, standardFont.family());
    defaultSettings->setFontSize(QWebSettings::DefaultFontSize, standardFont.pointSize());
    int minimumFontSize = settings.value(QLatin1String("minimumFontSize"),
                defaultSettings->fontSize(QWebSettings::MinimumFontSize)).toInt();
    defaultSettings->setFontSize(QWebSettings::MinimumFontSize, minimumFontSize);

    QString fixedFontFamily = defaultSettings->fontFamily(QWebSettings::FixedFont);
    int fixedFontSize = defaultSettings->fontSize(QWebSettings::DefaultFixedFontSize);
    QFont fixedFont = QFont(fixedFontFamily, fixedFontSize);
    fixedFont = qVariantValue<QFont>(settings.value(QLatin1String("fixedFont"), fixedFont));
    defaultSettings->setFontFamily(QWebSettings::FixedFont, fixedFont.family());
    defaultSettings->setFontSize(QWebSettings::DefaultFixedFontSize, fixedFont.pointSize());

    defaultSettings->setAttribute(QWebSettings::JavascriptEnabled, settings.value(QLatin1String("enableJavascript"), true).toBool());
    defaultSettings->setAttribute(QWebSettings::PluginsEnabled, settings.value(QLatin1String("enablePlugins"), true).toBool());
    defaultSettings->setAttribute(QWebSettings::AutoLoadImages, settings.value(QLatin1String("enableImages"), true).toBool());
    defaultSettings->setAttribute(QWebSettings::DeveloperExtrasEnabled, settings.value(QLatin1String("enableInspector"), false).toBool());

    QUrl url = settings.value(QLatin1String("userStyleSheet")).toUrl();
    defaultSettings->setUserStyleSheetUrl(url); */

    settings.endGroup();
}

QList<BrowserMainWindow*> BrowserApplication::mainWindows()
{
    clean();
    QList<BrowserMainWindow*> list;
    for (int i = 0; i < m_mainWindows.count(); ++i)
        list.append(m_mainWindows.at(i));
    return list;
}

void BrowserApplication::clean()
{
    // cleanup any deleted main windows first
    for (int i = m_mainWindows.count() - 1; i >= 0; --i)
        if (m_mainWindows.at(i).isNull())
            m_mainWindows.removeAt(i);
}

static const qint32 BrowserApplicationMagic = 0xec;

void BrowserApplication::saveSession()
{
    if (quiting)
        return;
    QSettings settings;
    settings.beginGroup(QLatin1String("MainWindow"));
    settings.setValue(QLatin1String("restoring"), false);
    settings.endGroup();

    /*
    QWebSettings *globalSettings = QWebSettings::globalSettings();
    if (globalSettings->testAttribute(QWebSettings::PrivateBrowsingEnabled))
        return;
    */
        
    clean();

    settings.beginGroup(QLatin1String("sessions"));

    int version = 2;

    QByteArray data;
    QBuffer buffer(&data);
    QDataStream stream(&buffer);
    buffer.open(QIODevice::WriteOnly);

    stream << qint32(BrowserApplicationMagic);
    stream << qint32(version);

    stream << qint32(m_mainWindows.count());
    for (int i = 0; i < m_mainWindows.count(); ++i)
        stream << m_mainWindows.at(i)->saveState();
    settings.setValue(QLatin1String("lastSession"), data);
    settings.endGroup();
}

bool BrowserApplication::canRestoreSession() const
{
    return !m_lastSession.isEmpty();
}

bool BrowserApplication::restoreLastSession()
{
    {
        QSettings settings;
        settings.beginGroup(QLatin1String("MainWindow"));
        if (settings.value(QLatin1String("restoring"), false).toBool()) {
            QMessageBox::information(0, tr("Restore failed"),
                tr("The saved session will not be restored because Arora crashed while trying to restore this session."));
            return false;
        }
        // saveSession will be called by an AutoSaver timer from the set tabs
        // and in saveSession we will reset this flag back to false
        settings.setValue(QLatin1String("restoring"), true);
    }
    int version = 2;
    QList<QByteArray> windows;
    QBuffer buffer(&m_lastSession);
    QDataStream stream(&buffer);
    buffer.open(QIODevice::ReadOnly);

    qint32 marker;
    qint32 v;
    stream >> marker;
    stream >> v;
    if (marker != BrowserApplicationMagic || v != version)
        return false;

    qint32 windowCount;
    stream >> windowCount;
    for (qint32 i = 0; i < windowCount; ++i) {
        QByteArray windowState;
        stream >> windowState;
        windows.append(windowState);
    }
    for (int i = 0; i < windows.count(); ++i) {
        BrowserMainWindow *newWindow = 0;
        if (i == 0 && m_mainWindows.count() >= 1) {
            newWindow = mainWindow();
        } else {
            newWindow = newMainWindow();
        }
        newWindow->restoreState(windows.at(i));
    }
    return true;
}

void BrowserApplication::installTranslator(const QString &name)
{
    QTranslator *translator = new QTranslator(this);
    translator->load(name, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    QApplication::installTranslator(translator);
}

#if defined(Q_WS_MAC)
bool BrowserApplication::event(QEvent* event)
{
    switch (event->type()) {
    case QEvent::ApplicationActivate: {
        clean();
        if (!m_mainWindows.isEmpty()) {
            BrowserMainWindow *mw = mainWindow();
            if (mw && !mw->isMinimized()) {
                mainWindow()->show();
            }
            return true;
        }
    }
    case QEvent::FileOpen:
        if (!m_mainWindows.isEmpty()) {
            mainWindow()->loadPage(static_cast<QFileOpenEvent *>(event)->file());
            return true;
        }
    default:
        break;
    }
    return QApplication::event(event);
}
#endif

void BrowserApplication::openUrl(const QUrl &url)
{
    mainWindow()->loadPage(url.toString());
}

BrowserMainWindow *BrowserApplication::newMainWindow()
{
    BrowserMainWindow *browser = new BrowserMainWindow();
    m_mainWindows.prepend(browser);
    browser->show();
    return browser;
}

BrowserMainWindow *BrowserApplication::mainWindow()
{
    clean();
    if (m_mainWindows.isEmpty())
        newMainWindow();
    return m_mainWindows[0];
}

DownloadManager *BrowserApplication::downloadManager()
{
    if (!s_downloadManager) {
        s_downloadManager = new DownloadManager();
    }
    return s_downloadManager;
}

HistoryManager *BrowserApplication::historyManager()
{
    if (!s_historyManager)
        s_historyManager = new HistoryManager();
    return s_historyManager;
}

BookmarksManager *BrowserApplication::bookmarksManager()
{
    if (!s_bookmarksManager) {
        s_bookmarksManager = new BookmarksManager;
    }
    return s_bookmarksManager;
}

QIcon BrowserApplication::icon(const QUrl &url)
{
    Q_UNUSED(url);
    QPixmap pixmap = QPixmap(QLatin1String(":defaulticon.png"));
    return pixmap;    
}

QString BrowserApplication::dataDirectory() const
{
    return applicationDirPath();
}
