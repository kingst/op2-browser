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
#include "webview.h"
#include "webpage.h"
#include "Message.h"
#include "OpDownloader.h"

#include <qbuffer.h>
#include <qclipboard.h>
#include <qdesktopservices.h>
#include <qevent.h>
#include <qmenu.h>
#include <qmessagebox.h>

#include <qmenubar.h>

#include <qwebframe.h>
#include <qwebpage.h>

#include <qdebug.h>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QApplication>
#include <QDataStream>
#include <QVariant>
#include <QBuffer>
#include <QPainter>
#include <QDesktopWidget>

#include <OpNetwork.h>
#include <QtUiTools/QUiLoader>
#include <qwebsettings.h>

#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <assert.h>

#include <iostream>
#include <QFile>

// I put this define in here so we could remove the optimization that
// loads the page in parallel with xembed stuff.  This should be
// commented out by default and should only be enabled for testing to
// show how much the optimization helps
//#define LOAD_POLICY_SERIAL

QByteArray  getByteArray(const QVariant& var) {
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::ReadWrite);

    QDataStream out(&buffer);
    out << var;
    return byteArray;
}


WebView::WebView(QWidget* parent)
    : QWebView(parent)
    , m_progress(0)
    , m_currentZoom(100)
    , m_page(new WebPage(this))
    , m_loaded(false)
    , m_finished(false)
    , m_userAction(false)
{
    Atom net_wm_state_skip_taskbar=XInternAtom (x11Info().display(),
                                                "_NET_WM_STATE_SKIP_TASKBAR", False);
    Atom net_wm_state_skip_pager=XInternAtom (x11Info().display(),
                                              "_NET_WM_STATE_SKIP_PAGER", False);
    Atom net_wm_state = XInternAtom (x11Info().display(),
                                     "_NET_WM_STATE", False);
    XChangeProperty (x11Info().display(), winId(), net_wm_state,
                     XA_ATOM, 32, PropModeAppend,
                     (unsigned char *)&net_wm_state_skip_taskbar, 1);
    XChangeProperty (x11Info().display(), winId(), net_wm_state,
                     XA_ATOM, 32, PropModeAppend,
                     (unsigned char *)&net_wm_state_skip_pager, 1);

    setPage(m_page);
    connect(page(), SIGNAL(statusBarMessage(const QString&)),
            SLOT(setStatusBarText(const QString&)));
    connect(this, SIGNAL(loadProgress(int)),
            this, SLOT(setProgress(int)));
    connect(this, SIGNAL(loadFinished(bool)),
            this, SLOT(loadFinished()));
    connect(page(), SIGNAL(loadingUrl(const QUrl&)),
            this, SIGNAL(urlChanged(const QUrl &)));
    connect(page(), SIGNAL(downloadRequested(const QNetworkRequest &)),
            this, SLOT(downloadRequested(const QNetworkRequest &)));
    page()->setForwardUnsupportedContent(true);

    // void loadStarted();
    connect(this, SIGNAL(loadStarted()),
            this, SLOT(loadStarted()));
    
    //void loadProgress(int progress);
    connect(this, SIGNAL(loadProgress(int)),
            this, SLOT(loadProgress(int)));
    
    //void loadFinished(bool ok);
    connect(this, SIGNAL(loadFinished(bool)),
            this, SLOT(loadFinished(bool)));
    
    //void linkHovered(const QString &link, const QString &title, const QString &textContent);
    connect(page(), SIGNAL(linkHovered(const QString&, const QString&, const QString&)),
            this, SLOT(linkHovered(const QString&, const QString&, const QString&)));
    
    //void statusBarMessage(const QString& text);
    connect(page(), SIGNAL(statusBarMessage(const QString&)),
            this, SLOT(statusBarMessage(const QString&)));
    
    //void geometryChangeRequested(const QRect& geom);
    connect(page(), SIGNAL(geometryChangeRequested(const QRect&)),
            this, SLOT(geometryChangeRequested(const QRect&)));
    
    //void windowCloseRequested();
    connect(page(), SIGNAL(windowCloseRequested()),
            this, SLOT(windowCloseRequested()));

    //void toolBarVisibilityChangeRequested(bool visible);
    connect(page(), SIGNAL(toolBarVisibilityChangeRequested(bool)),
            this, SLOT(toolBarVisibilityChangeRequested(bool)));
    
    //void statusBarVisibilityChangeRequested(bool visible);
    connect(page(), SIGNAL(statusBarVisibilityChangeRequested(bool)),
            this, SLOT(statusBarVisibilityChangeRequested(bool)));
    
    //void menuBarVisibilityChangeRequested(bool visible);
    connect(page(), SIGNAL(menuBarVisibilityChangeRequested(bool)),
            this, SLOT(menuBarVisibilityChangeRequested(bool)));

    //void titleChanged(const QString& title);
    connect(this, SIGNAL(titleChanged(const QString&)),
            this, SLOT(titleChanged(const QString&)));

    //void iconChanged();
    connect(this, SIGNAL(iconChanged()),
            this, SLOT(iconChanged()));
    
    //void urlChanged(const QUrl &url);
    connect(this, SIGNAL(urlChanged(const QUrl&)),
            this, SLOT(urlChanged(const QUrl&)));


    
    // the zoom values (in percent) are chosen to be like in Mozilla Firefox 3
    m_zoomLevels << 30 << 50 << 67 << 80 << 90;
    m_zoomLevels << 100;
    m_zoomLevels << 110 << 120 << 133 << 150 << 170 << 200 << 240 << 300;

    OPNET::OpNetwork::instance()->init(winId());
    connect((OPNET::OpNetwork::instance()), SIGNAL(newUrl(int, const QByteArray&)),
            this, SLOT(setUrl(int, const QByteArray&)));
    connect((OPNET::OpNetwork::instance()), SIGNAL(webAppMsg(int, const QByteArray&)),
            this, SLOT(slotWebAppMsg(int, const QByteArray&)));

    if(!IS_CRAWLER && !IS_REPLICA) {
        m_page->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    } else if(IS_REPLICA) {
        showMinimized();
    }
}

void WebView::setUrl(int uid, const QByteArray& content) {
    Q_UNUSED(uid);

    decodedNewUrlMsg(content, m_request, m_postData);
    m_initialUrl = m_request.url();

    // XXX I moved the loadPolicy call here instead of at the webapp msg
    // to make sure we were overalpping some of the xembed stuff with the actual
    // loading of the web page for faster performance.  I am not sure if this
    // is going to affect the correctness at all
#if !defined(LOAD_POLICY_SERIAL)
    loadPolicy(m_request, m_postData, ThisWebApp);
#endif
    // tricky here, show after get the new url
    resize(1, 1);
    QRect rect = QApplication::desktop()->screenGeometry();
    move(rect.bottomRight().x()+ 100, rect.bottomRight().y() + 100);
    show();
}

void WebView::snapshot(QString path)
{
    if (path.size() == 0) {
        //if (OPNET::OpNetwork::instance()->isReplay())
        path = OPNET::OpNetwork::instance()->getName() + ".PNG";
        //else
        //path = url().encodedHost() + ".PNG";
    }
    std::cerr << "Snapshot to: " << path.toLatin1().constData() << std::endl;
    WebView* view = this;
    QImage result = QImage(view->size(), QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);

    QPainter p(&result);
    view->render(&p);
    p.setPen(QPen(Qt::darkGray, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(result.rect().adjusted(0, 0, -1, -1));
    p.end();
    result.save(path);

    QFile file1(OPNET::OpNetwork::instance()->getName() + ".DUMP1");
    file1.open(QIODevice::WriteOnly | QIODevice::Text);
    QString dump1 = this->page()->mainFrame()->renderTreeDump();
    file1.write(dump1.toLatin1().constData(), dump1.toLatin1().size());
    file1.close();
    QFile file2(OPNET::OpNetwork::instance()->getName() + ".DUMP2");
    file2.open(QIODevice::WriteOnly | QIODevice::Text);
    QString dump2 = this->page()->mainFrame()->toPlainText();
    file2.write(dump2.toLatin1().constData(), dump2.toLatin1().size());
    file2.close();
    QFile file3(OPNET::OpNetwork::instance()->getName() + ".DUMP3");
    file3.open(QIODevice::WriteOnly | QIODevice::Text);
    QString dump3 = this->page()->mainFrame()->toHtml();
    file3.write(dump3.toLatin1().constData(), dump3.toLatin1().size());
    file3.close(); 
    
}

void WebView::slotWebAppMsg(int type, const QByteArray& content)
{
    static int count = 0;
    QDataStream stream(content);
    QVariant var;
    QRect rect = QApplication::desktop()->screenGeometry();
    
    if (content.size() > 0)
        stream >> var;

    switch (type) {
    case MSG_WEBAPP_CLOSE:
        m_finished = true;
        close();

        break;
    case MSG_SNAP_SHOT:
        snapshot(QString(content));
        break;
    case MSG_WEBAPP_SHOW:
        show();
#if defined(LOAD_POLICY_SERIAL)
        loadPolicy(m_request, m_postData, ThisWebApp);
#endif
        if (content.size() > 0)
            resize(var.toSize());
        break;
    case MSG_WEBAPP_HIDE:
        resize(1, 1);
        move(rect.bottomRight().x()+ 100, rect.bottomRight().y() + 100);
        hide();
        break;
    case MSG_WEBAPP_LOAD_URL:
        loadUrl(QNetworkRequest(QUrl(content)), QNetworkAccessManager::GetOperation, QByteArray());
        break;
    default:
        qDebug() << "Unsupported WebApp Msg.";
        break;
    }
}

void WebView::closeEvent(QCloseEvent *event)
{
    if (!m_finished) {
        OPNET::OpNetwork::instance()->sendUIMsg(MSG_webAppExited, QByteArray());
        event->ignore();
    } else {
        QWidget::closeEvent(event);
    }
}

void WebView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = new QMenu(this);

    QWebHitTestResult r = page()->mainFrame()->hitTestContent(event->pos());

    if (!r.linkUrl().isEmpty()) {
        menu->addAction(tr("&Open"), this, SLOT(openLink()));
        menu->addAction(tr("Open in New &Window"), this, SLOT(openLinkInNewWindow()))->setData(r.linkUrl().toString());
        menu->addAction(tr("Open in New &Tab"), this, SLOT(openLinkInNewTab()))->setData(r.linkUrl().toString());
        menu->addSeparator();
        menu->addAction(tr("Save Lin&k"), this, SLOT(downloadLinkToDisk()));
        menu->addAction(tr("&Bookmark This Link"), this, SLOT(bookmarkLink()))->setData(r.linkUrl().toString());
        menu->addSeparator();
        menu->addAction(tr("&Copy Link Location"), this, SLOT(copyLinkToClipboard()));
        if (page()->settings()->testAttribute(QWebSettings::DeveloperExtrasEnabled))
            menu->addAction(pageAction(QWebPage::InspectElement));
    }

    if (!r.imageUrl().isEmpty()) {
        if (!menu->isEmpty())
            menu->addSeparator();
        menu->addAction(tr("Open Image in New &Window"), this, SLOT(openImageInNewWindow()))->setData(r.imageUrl().toString());
        menu->addAction(tr("Open Image in New &Tab"), this, SLOT(openImageInNewTab()))->setData(r.imageUrl().toString());
        menu->addSeparator();
        menu->addAction(tr("&Save Image"), this, SLOT(downloadImageToDisk()));
        menu->addAction(tr("&Copy Image"), this, SLOT(copyImageToClipboard()));
        menu->addAction(tr("C&opy Image Location"), this, SLOT(copyImageLocationToClipboard()))->setData(r.imageUrl().toString());
    }

#ifdef WEBKIT_TRUNK // i.e. Qt 4.5, but not in Qt 4.5 yet
    if (menu->isEmpty())
        menu = page()->createStandardContextMenu();
#endif

    if (!menu->isEmpty()) {
        /*if (m_page->mainWindow()->menuBar()->isHidden()) {
          menu->addSeparator();
          menu->addAction(m_page->mainWindow()->showMenuBarAction());
          }*/

        menu->exec(mapToGlobal(event->pos()));
        delete menu;
        return;
    }
    delete menu;

    QWebView::contextMenuEvent(event);
}

void WebView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        int numDegrees = event->delta() / 8;
        int numSteps = numDegrees / 15;
        setTextSizeMultiplier(textSizeMultiplier() + numSteps * 0.1);
        event->accept();
        return;
    }
    QWebView::wheelEvent(event);
}

void WebView::resizeEvent(QResizeEvent *event)
{
    int offset = event->size().height() - event->oldSize().height();
    int currentValue = page()->mainFrame()->scrollBarValue(Qt::Vertical);
    setUpdatesEnabled(false);
    page()->mainFrame()->setScrollBarValue(Qt::Vertical, currentValue - offset);
    setUpdatesEnabled(true);
    QWebView::resizeEvent(event);
}

void WebView::openLink()
{
    m_userAction = true;
    pageAction(QWebPage::OpenLink)->trigger();
}

void WebView::openLinkInNewTab()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        QUrl url = QUrl(action->data().toString());
        QNetworkRequest request;
        request.setUrl(url);
        loadPolicy(request, QByteArray(), NewTabNoSelect);
    }
}

void WebView::openLinkInNewWindow()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        QUrl url = QUrl(action->data().toString());
        QNetworkRequest request;
        request.setUrl(url);
        loadPolicy(request, QByteArray(), NewWindow);
    }
}

void WebView::downloadLinkToDisk()
{
    pageAction(QWebPage::DownloadLinkToDisk)->trigger();
}

void WebView::copyLinkToClipboard()
{
    pageAction(QWebPage::CopyLinkToClipboard)->trigger();
}

void WebView::openImageInNewTab()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        QUrl url = QUrl(action->data().toString());
        QNetworkRequest request;
        request.setUrl(url);
        loadPolicy(request, QByteArray(), NewTabNoSelect);
    }
}

void WebView::openImageInNewWindow()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        QUrl url = QUrl(action->data().toString());
        QNetworkRequest request;
        request.setUrl(url);
        loadPolicy(request, QByteArray(), NewWindow);
    }
}

void WebView::downloadImageToDisk()
{
    pageAction(QWebPage::DownloadImageToDisk)->trigger();
}

void WebView::copyImageToClipboard()
{
    pageAction(QWebPage::CopyImageToClipboard)->trigger();
}

void WebView::copyImageLocationToClipboard()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        QApplication::clipboard()->setText(action->data().toString());
    }
}

void WebView::bookmarkLink()
{
    
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        //AddBookmarkDialog dialog(action->data().toString(), QString());
        //dialog.exec();

        std::cerr << "Not Impl" << std::endl; // addbookmark
        Q_UNUSED(action);
    }
}

void WebView::setProgress(int progress)
{
    m_progress = progress;
}

void WebView::zoomIn()
{
    int i = m_zoomLevels.indexOf(m_currentZoom);
    Q_ASSERT(i >= 0);

    if (i < m_zoomLevels.count() - 1)
        m_currentZoom = m_zoomLevels[i + 1];

    setTextSizeMultiplier(qreal(m_currentZoom) / 100.0);
}

void WebView::zoomOut()
{
    int i = m_zoomLevels.indexOf(m_currentZoom);
    Q_ASSERT(i >= 0);

    if (i > 0)
        m_currentZoom = m_zoomLevels[i - 1];

    setTextSizeMultiplier(qreal(m_currentZoom) / 100.0);
}

void WebView::resetZoom()
{
    m_currentZoom = 100;
    setTextSizeMultiplier(1.0);
}

void WebView::loadFinished()
{
    if (100 != m_progress) {
        qWarning() << "Recieved finished signal while progress is still:" << progress()
                   << "Url:" << url();
    }
    m_loaded = true;
    m_progress = 0;
}

void WebView::loadPolicy(const QNetworkRequest &request, const QByteArray& body, OpenType type)
{
    QNetworkAccessManager::Operation op = QNetworkAccessManager::GetOperation;
    if (body.size() > 0)
        op = QNetworkAccessManager::PostOperation;
        
    if (type == ThisWebApp) {
        loadUrl(request, op, body);
    } else if(type == NewSubFrame) {
        OPNET::OpNetwork::instance()->sendSysCall(MSG_NEW_URL, type, encodedNewUrlMsg(request, body));        
    } else {
        OPNET::OpNetwork::instance()->sendSysCall(MSG_NEW_URL, type, encodedNewUrlMsg(request, body));
    }
}


QByteArray WebView::encodedNewUrlMsg(const QNetworkRequest& request, const QByteArray& body)
{
    QByteArray url = request.url().toString().toLatin1();
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::ReadWrite);

    QDataStream out(&buffer);

    QVariant var = request.header(QNetworkRequest::ContentTypeHeader);
    out << var;
    var = QVariant(body);
    out << var;

    byteArray = url + (char)0 + byteArray;
    return byteArray;
}

void WebView::decodedNewUrlMsg(const QByteArray& msg, QNetworkRequest& request, QByteArray& body)
{
    QString url = QString(msg.constData());
    QByteArray data = msg.mid(url.size() + 1);
    QDataStream stream(data);
    QVariant contentType;
    QVariant postData;
    if (data.size() > 0) {
        stream >> contentType;
        stream >> postData;
    }
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    body = postData.toByteArray(); 
}


/*void WebView::loadUrl(const QUrl &url, const QString &title)
  {
  if (url.scheme() == QLatin1String("javascript")) {
  QString scriptSource = url.toString().mid(11);
  QVariant result = page()->mainFrame()->evaluateJavaScript(scriptSource);
  if (result.canConvert(QVariant::String)) {
  QString newHtml = result.toString();
  setHtml(newHtml);
  }
  return;
  }
  m_initialUrl = url;
  if (!title.isEmpty())
  emit titleChanged(tr("Loading..."));
  else
  emit titleChanged(title);
  load(url);
  }*/

void WebView::setHtml(const QString &html, const QUrl &baseUrl) {
    Q_UNUSED(html);
    Q_UNUSED(baseUrl);
    return;
}

void WebView::setContent ( const QByteArray & data, const QString & mimeType, const QUrl & baseUrl) {
    Q_UNUSED(data);
    Q_UNUSED(mimeType);
    Q_UNUSED(baseUrl);
    return;
}

void WebView::load(const QNetworkRequest &request, QNetworkAccessManager::Operation operation, const QByteArray &body) {
    qDebug() << "load " << request.url();
    QWebView::load(request, operation, body);
}

void WebView::load(QUrl &url) {
    qDebug() << "not expecting load without request, etc args, bailing...";
    Q_UNUSED(url);
    return;
}

void WebView::loadUrl(const QNetworkRequest &request, QNetworkAccessManager::Operation operation, const QByteArray &body)
{
    m_initialUrl = request.url();
    QWebView::load(request, operation, body);
}

QString WebView::lastStatusBarText() const
{
    return m_statusBarText;
}

QUrl WebView::url() const
{
    QUrl url = QWebView::url();
    if (!url.isEmpty())
        return url;

    return m_initialUrl;
}

void WebView::mousePressEvent(QMouseEvent *event)
{
    m_page->m_pressedButtons = event->buttons();
    m_page->m_keyboardModifiers = event->modifiers();
    QWebView::mousePressEvent(event);
}

void WebView::mouseReleaseEvent(QMouseEvent *event)
{
    QWebView::mouseReleaseEvent(event);
    if (!event->isAccepted() && (m_page->m_pressedButtons & Qt::MidButton)) {
        QUrl url(QApplication::clipboard()->text(QClipboard::Selection));
        if (!url.isEmpty() && url.isValid() && !url.scheme().isEmpty()) {
            //loadUrl(url);
            QNetworkRequest request;
            request.setUrl(url);
            loadPolicy(request, QByteArray(), NewWebApp);
        }
    }
}

void WebView::setStatusBarText(const QString &string)
{
    m_statusBarText = string;
}

void WebView::downloadRequested(const QNetworkRequest &request)
{
    new OpDownloader(request);    
}


void WebView::loadStarted() {
    OPNET::OpNetwork::instance()->sendUIMsg(MSG_loadStarted, QByteArray());
}

void WebView::loadProgress(int progress) {
    QVariant var(progress);
    OPNET::OpNetwork::instance()->sendUIMsg(MSG_loadProgress, getByteArray(var));
}

void WebView::loadFinished(bool ok) {
    QVariant var(ok);
    OPNET::OpNetwork::instance()->sendUIMsg(MSG_loadFinished, getByteArray(var));
}

void WebView::linkHovered(const QString &link, const QString &title, const QString &textContent) {
    Q_UNUSED(title);
    Q_UNUSED(textContent);
    QVariant var(link);
    OPNET::OpNetwork::instance()->sendUIMsg(MSG_linkHovered, getByteArray(var));
}

void WebView::statusBarMessage(const QString& text) {
    QVariant var(text);
    OPNET::OpNetwork::instance()->sendUIMsg(MSG_statusBarMessage, getByteArray(var));
}

void WebView::geometryChangeRequested(const QRect& geom) {
    QVariant var(geom);
    OPNET::OpNetwork::instance()->sendUIMsg(MSG_geometryChangeRequested, getByteArray(var));
}

void WebView::windowCloseRequested() {
    OPNET::OpNetwork::instance()->sendUIMsg(MSG_windowCloseRequested, QByteArray());
}

void WebView::toolBarVisibilityChangeRequested(bool visible) {
    QVariant var(visible);
    OPNET::OpNetwork::instance()->sendUIMsg(MSG_toolBarVisibilityChangeRequested, getByteArray(var));
}

void WebView::statusBarVisibilityChangeRequested(bool visible) {
    QVariant var(visible);
    OPNET::OpNetwork::instance()->sendUIMsg(MSG_statusBarVisibilityChangeRequested, getByteArray(var));
}

void WebView::menuBarVisibilityChangeRequested(bool visible) {
    QVariant var(visible);
    OPNET::OpNetwork::instance()->sendUIMsg(MSG_menuBarVisibilityChangeRequested, getByteArray(var));
}

void WebView::titleChanged(const QString& title) {
    QVariant var(title);
    OPNET::OpNetwork::instance()->sendUIMsg(MSG_titleChanged, getByteArray(var));
}

void WebView::iconChanged() {
    OPNET::OpNetwork::instance()->sendUIMsg(MSG_iconChanged, QByteArray());
}

void WebView::urlChanged(const QUrl &url) {
    QVariant var(url);
    OPNET::OpNetwork::instance()->sendUIMsg(MSG_urlChanged, getByteArray(var));
}
