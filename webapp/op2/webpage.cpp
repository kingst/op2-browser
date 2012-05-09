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
#include "webpage.h"
#include "webview.h"
#include "OpNetwork.h"
#include "jswindow.h"
#include "OpDownloader.h"
#include "FrameInit.h"
#include "Message.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

#include <QtUiTools/QtUiTools>
#include <QBuffer>
#include <QDataStream>

#include <QNetworkReply>
#include <qwebframe.h>
#include <qwebelement.h>


WebPage::WebPage(QObject *parent)
    : QWebPage(parent)
    , m_keyboardModifiers(Qt::NoModifier)
    , m_pressedButtons(Qt::NoButton)
    , m_openInNewTab(false)
    , m_posted(false)
    , m_back(false)
    , m_iframeHandled(IFRAME_HANDLED)
{
    setNetworkAccessManager(OPNET::OpNetwork::instance()->networkAccessManager());
    connect(this, SIGNAL(unsupportedContent(QNetworkReply *)),
            this, SLOT(handleUnsupportedContent(QNetworkReply *)));

    connect(OPNET::OpNetwork::instance(), SIGNAL(userAction(int, const QByteArray&)),
            this, SLOT(replayEvent(int, const QByteArray&)));

    if (m_iframeHandled) {
        connect(this, SIGNAL(frameCreated(QWebFrame *)),
                this, SLOT(handleFrameCreated(QWebFrame *)));
    }
}

#ifdef OP_EXTENDED
bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type, const QByteArray& postData)
{
    QString scheme = request.url().scheme();
    if (scheme == QLatin1String("mailto")
        || scheme == QLatin1String("ftp")) {
        //QDesktopServices::openUrl(request.url());
        return false;
    }

    if (IS_CRAWLER) // when crawler, we run the webapplication is the same process
        // make it easier for replay
        return QWebPage::acceptNavigationRequest(frame, request, type, postData);
    
    WebView::OpenType t = WebView::NewWebApp;
    WebView* view = qobject_cast<WebView*>(parent());
    Q_ASSERT(view != 0);

    // little trick to avoid double post
    if (m_posted) {
        m_posted = false;
    } else if (postData.size() > 0) {
        m_posted = true;
    }
    // ctrl open in new tab
    // ctrl-shift open in new tab and select
    // ctrl-alt open in new window
    if (type == QWebPage::NavigationTypeLinkClicked
        && (m_keyboardModifiers & Qt::ControlModifier
            || m_pressedButtons == Qt::MidButton)) {
        bool newWindow = (m_keyboardModifiers & Qt::AltModifier);
        if (newWindow) {
            t = WebView::NewWindow;            
        } else {
            bool selectNewTab = (m_keyboardModifiers & Qt::ShiftModifier);
            if (selectNewTab)
                t = WebView::NewTabSelect;
            else
                t = WebView::NewTabNoSelect;
        }

        // check and load
        view->loadPolicy(request, postData, t);
        
        m_keyboardModifiers = Qt::NoModifier;
        m_pressedButtons = Qt::NoButton;
        return false;
    }
    QUrl::FormattingOptions format = QUrl::RemoveQuery | QUrl::RemoveFragment | QUrl::StripTrailingSlash;
    if (frame == mainFrame()) {
        m_loadingUrl = request.url();
        QUrl current = view->url();
        bool sop = (m_loadingUrl.scheme().toLower() == current.scheme().toLower())
            && (m_loadingUrl.host().toLower() == current.host().toLower())
            && (m_loadingUrl.port() == current.port());
        if (!sop) {
            t = WebView::NewWebApp;
            
            // check and load
            if (m_posted)
                view->loadPolicy(request, postData, t);
            else
                view->loadPolicy(request, QByteArray(), t);

            return false;
        } else {
            emit loadingUrl(m_loadingUrl);
            
            
            if ((type != NavigationTypeOther) && (type != NavigationTypeReload) && (type != NavigationTypeBackOrForward)
                || (view->m_userAction)) {
                QVariant var(m_loadingUrl.toString());
                OPNET::OpNetwork::instance()->sendUIMsg(MSG_addHistoryItem, getByteArray(var));
                view->m_userAction = false;
            }
            if (type == NavigationTypeBackOrForward) {
                m_back = false;
            }
        }
    }
    else if(frame == 0)
    {
        t = WebView::NewTabNoSelect;

        // check and load
        view->loadPolicy(request, postData, t);

        return false;
    }
    // same-origin and "about:blank" navigation requests fall-through to acceptNavigationRequest
    else if( m_iframeHandled &&
             (request.url().toString(format | QUrl::RemovePath) != view->url().toString(format | QUrl::RemovePath) 
              && request.url().toString() != "about:blank")) {
        //qDebug() << "navigation request for sub-frame \"" << frame->frameName() << "\" to " << request.url() << " type: " << type;
        t = WebView::NewSubFrame;
        OPNET::OpNetwork::instance()->sendSysCall(MSG_EMBED_FRAME, 0, frame->frameName().toAscii());
        view->loadPolicy(request, postData, t);
        return false;
    }

    return QWebPage::acceptNavigationRequest(frame, request, type, postData);
}
#endif

void WebPage::triggerAction(WebAction action, bool checked)
{
    if (action == Back) {
        m_back = true;
        QVariant var(m_back);
        OPNET::OpNetwork::instance()->sendUIMsg(MSG_navBackOrForward, getByteArray(var));
    }
    if (action == Forward) {
        m_back = false;
        QVariant var(m_back);
        OPNET::OpNetwork::instance()->sendUIMsg(MSG_navBackOrForward, getByteArray(var));
    }

    QWebPage::triggerAction(action, checked);
}

QWebPage *WebPage::createWindow(QWebPage::WebWindowType type)
{
    Q_UNUSED(type);
    if (IS_CRAWLER)
	return 0 ;
    // it reaches here if JavaScript tries to create a new window
    JSWindow* page = new JSWindow();
    WebView* view = qobject_cast<WebView*>(parent());
    page->setView(view);
    return page;
}

#if !defined(QT_NO_UITOOLS)
QObject *WebPage::createPlugin(const QString &classId, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues)
{
    Q_UNUSED(url);
    Q_UNUSED(paramNames);
    Q_UNUSED(paramValues);
    QUiLoader loader;
    return loader.createWidget(classId, view());
}
#endif // !defined(QT_NO_UITOOLS)

void WebPage::handleFrameCreated(QWebFrame *frame) {
    //qDebug() << "frame created = " << frame->frameName() << " " << frame->url();
    FrameInit *frameInit = new FrameInit(frame);
    connect(frame, SIGNAL(urlChanged(const QUrl &)),
            frameInit, SLOT(handleUrlChanged(const QUrl &)));
    connect(frame, SIGNAL(initialLayoutCompleted()),
            frameInit, SLOT(handleFrameLayoutComplete()));
}

void WebPage::handleUnsupportedContent(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        if (reply->header(QNetworkRequest::ContentTypeHeader).isValid()) {
            new OpDownloader(reply);
        }
        return;
    }

    if (reply->error() == QNetworkReply::ProtocolUnknownError) {
        // we currently don't support protocol other than http(s):// and file://
        // return;
    }

    //display notfound
    if (reply->url().isEmpty())
        return;
    QFile file(QLatin1String(":/notfound.html"));
    bool isOpened = file.open(QIODevice::ReadOnly);
    Q_ASSERT(isOpened);
    QString title = ("HTTP 404 Not Found");
    QString html = QString(QLatin1String(file.readAll()));
    
    QList<QWebFrame*> frames;
    frames.append(mainFrame());
    while (!frames.isEmpty()) {
        QWebFrame *frame = frames.takeFirst();
        if (frame->url() == reply->url()) {
            frame->setHtml(html, reply->url());
            return;
        }
        QList<QWebFrame *> children = frame->childFrames();
        foreach (QWebFrame *frame, children)
            frames.append(frame);
    }
    if (m_loadingUrl == reply->url()) {
        mainFrame()->setHtml(html, reply->url());
    }
}


bool WebPage::event(QEvent *ev)
{
    QMouseEvent* mouseEvent = NULL;
    QWheelEvent* wheelEvent = NULL;
    QKeyEvent* keyEvent = NULL;
    
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::ReadWrite);

    QDataStream out(&buffer);

    out << (int) ev->type();
    
    switch (ev->type()) {
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonRelease:
        if (OPNET::OpNetwork::instance()->isReplay())
            return false;
        mouseEvent = (static_cast<QMouseEvent*>(ev));
        out << mouseEvent->pos() << mouseEvent->globalPos()
            << (int) mouseEvent->button() << (int) mouseEvent->buttons()
            << (int) mouseEvent->modifiers();
        OPNET::OpNetwork::instance()->sendSysCall(MSG_LOG_USER_INPUT, (int) ev->type(), byteArray);
        break;
    case QEvent::Wheel:
        if (OPNET::OpNetwork::instance()->isReplay())
            return false;;
        wheelEvent = (static_cast<QWheelEvent*>(ev));
        out << wheelEvent->pos() << wheelEvent->globalPos()
            << wheelEvent->delta() << (int) wheelEvent->buttons()
            << (int) wheelEvent->modifiers() << (int) wheelEvent->orientation();
        OPNET::OpNetwork::instance()->sendSysCall(MSG_LOG_USER_INPUT, (int) ev->type(), byteArray);
        break;
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        if (OPNET::OpNetwork::instance()->isReplay())
            return false;
        keyEvent = (static_cast<QKeyEvent*>(ev));
        out << keyEvent->key() << (int) keyEvent->modifiers()
            << keyEvent->text() << keyEvent->isAutoRepeat()
            << (int) keyEvent->count();
        OPNET::OpNetwork::instance()->sendSysCall(MSG_LOG_USER_INPUT, (int) ev->type(), byteArray);
        break;
    default:
        break;
    }

    return QWebPage::event(ev);
}


bool WebPage::replayEvent(int type, const QByteArray& args)
{

    QDataStream stream(args);
    int _type;
    stream >> _type;
    Q_ASSERT(type == _type);
    
    switch (type) {
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonRelease:
    {
        QPoint pos;
        QPoint globalPos;
        int button;
        int buttons;
        int modifiers;
        stream >> pos >> globalPos >> button >> buttons >> modifiers;
        QMouseEvent mouseEvent((QEvent::Type) type, pos, globalPos,
                           (Qt::MouseButton) button,
                           (Qt::MouseButtons) buttons,
                           (Qt::KeyboardModifiers) modifiers);
        QWebPage::event(&mouseEvent);
        break;
    }
    case QEvent::Wheel:
    {
        QPoint pos;
        QPoint globalPos;
        int delta;
//        Qt::MouseButtons
        int buttons;
//        Qt::KeyboardModifiers
        int modifiers;
//        Qt::Orientation
        int orient;

        stream >> pos >> globalPos >> delta >> buttons
               >> modifiers >> orient;

        QWheelEvent wheelEvent(pos, globalPos, delta,
                               (Qt::MouseButtons)buttons,
                               (Qt::KeyboardModifiers)modifiers,
                               (Qt::Orientation)orient);
        QWebPage::event(&wheelEvent);        
        break;
    }
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    {
        int key;
        int modifiers;
        QString text;
        bool autorep;
        int count;
        stream >> key >> modifiers >> text >> autorep >> count;
        QKeyEvent keyEvent((QEvent::Type)type, key,
                           (Qt::KeyboardModifiers) modifiers,
                           text, autorep, count);

        QWebPage::event(&keyEvent);
        break;
    }
    default:
        std::cerr << "Unhandled user event type" << std::endl;
        return false;
    }
    return true;
}
