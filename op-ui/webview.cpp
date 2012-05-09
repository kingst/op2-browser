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

#include <QUrl>
#include <QString>
#include <QDataStream>
#include <QVariant>
#include <QAction>
#include <QBuffer>
#include <QDebug>

#include "Message.h"
#include "webview.h"
#include "history.h"
#include "frameview.h"

QByteArray static getByteArray(const QVariant& var)
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::ReadWrite);

    QDataStream out(&buffer);
    out << var;
    return byteArray;
}


WebView::WebView(QWidget* parent)
    : QX11EmbedContainer(parent)
    , m_containerId( winId() )
    , m_clientId(0)
    , m_clientWId(0)
    , m_nextClientWId(0)
    , m_lastClientWId(0)
    , m_embedding(false)
    , m_progress(0)
    , m_history(new History(this))
{
    connect(this, SIGNAL(clientClosed()),
            this, SLOT(slotClientClosed()));
    connect(this, SIGNAL(clientIsEmbedded()),
            this, SLOT(slotClientIsEmbeded()));

    memset(m_actions, 0, sizeof(m_actions));
}

WebView::~WebView()
{
    if ( m_clientId != 0 ) {
        sendSysCall(MSG_WEBAPP_CLOSE, QByteArray::number(m_clientId));
        closeFrames();
    } 
}

void WebView::setClientIds(int clientId, int clientWId)
{
    m_clientId = clientId;
    m_clientWId = clientWId;
    sendSysCall(MSG_UPDATA_CONTAINER, QByteArray::number(m_clientId));
}

void WebView::handleEmbed(Message* msg) {
    int clientId = msg->getSrcId();
    int clientWId = msg->getMsgValue();
    if (clientId == 0 || clientWId == 0)
        return;

    m_embedding = true;
    int oldClientId = m_clientId;
    setClientIds(clientId, clientWId);
    if (oldClientId != 0) {
        // close the old one
        sendSysCall(MSG_WEBAPP_CLOSE, QByteArray::number(oldClientId));
        closeFrames();
        // if one is embeded, discard it
        if (clientWinId() != 0) {
            discardClient();
        }
        // if ui is embedding the old one, we will discard it in slotClientIsEmbeded()
    } else {
        embedClient(clientWId);
    }
}

void WebView::reEmbedClient() {
    if(!m_embedding) {
        qDebug() << "re-embed the somehow closed client";
        embedClient(m_clientWId);
    } else {
        qDebug() << "the m_clientWId changed during the timer, ignore reEmbedClient";
    }
}    

void WebView::slotClientClosed() {
    if (m_embedding) {
        embedClient(m_clientWId);
    } else {
        // re-embed somewhow closed client
        qDebug() << "Warning: ignoring clientClosed signal (Shuo, can you look at this)";
        m_lastClientWId = m_clientWId;
        QTimer::singleShot(100, this, SLOT(reEmbedClient()));
    }
}

void WebView::slotClientIsEmbeded()
{
    if (m_clientWId != (int) clientWinId()) {
        // client changed when the old one is being embeded
        discardClient();
    } else {
        m_embedding = false;
        // set the size
        QVariant var(size());
        sendWebAppMsg(MSG_WEBAPP_SHOW, getByteArray(var));
    }
}


void WebView::prepFrame(Message* msg)
{ 
    int clientId = msg->getSrcId();
    int winId = msg->getMsgValue();
    if (winId == 0 || clientId == 0)
        return;

    //qDebug() << "prepFrame entering"; // WID: " << m_iframewid.size() << " pending Widgets " << m_iframewidget.size();

    m_iframewids.append(winId);
    m_iframeclientids.append(clientId);
    //qDebug() << "prepFrame leaving"; // WID: " << m_iframewid.size() << " pending Widgets " << m_iframewidget.size();
}

void WebView::embedFrame(Message* msg)
{
    QByteArray data((char *)msg->getMsgData(), msg->getDataLen());
    QVariant var(data);

    //qDebug() << "embedFrame entering"; // WID: " << m_iframewid.size() << " pending Widgets " << m_iframewidget.size();

    if(msg->getMsgValue() == 0)
    {
        QString frameName = var.toString();
        //qDebug() << "\tnaming container " << frameName;
        m_iframenames.append(frameName);
    }
    else if(msg->getMsgValue() == 1)
    {
        QString strData = var.toString();
        QStringList frameattrs = strData.split(tr(","));

        if(frameattrs.size() != 5)
        {
            qDebug() << "error in msg to UI for embedFrame!" << data;
            return;
        }

        int height, width, x, y;
        height = frameattrs.at(0).toInt();
        width = frameattrs.at(1).toInt();
        x = frameattrs.at(2).toInt();
        y = frameattrs.at(3).toInt();
        QString frameName = frameattrs.at(4);

        if(m_iframenames.contains(frameName)) 
        {
            //qDebug() << "\tembed iframe from (" << x << ", " << y << ") "   
            //    << "to (" << x+width << ", " << y+height << ") with name (" << frameName << ")";   

            QWidget* frameParent = new QWidget(this->parentWidget());
            FrameView* qxec = new FrameView(frameParent);

            frameParent->setGeometry(x,y,width,height);

            QVBoxLayout *layout = new QVBoxLayout;
            layout->setSpacing(0);
            layout->setContentsMargins(0, 0, 0, 0);

            layout->addWidget(qxec);
            frameParent->setLayout(layout);

            frameParent->show();
            qxec->show();

            int clientId = m_iframeclientids.takeFirst();
            int clientWid = m_iframewids.takeFirst();
            qxec->handleEmbed(clientId, clientWid);

            m_iframes.push_back(qxec);
        }
    } else {
        FrameView* view;
        int containerId = atoi(msg->getStringData().c_str());
        int clientWId = msg->getMsgValue();
        for (QList<QWidget*>::iterator iter = m_iframes.begin();
             iter != m_iframes.end(); ++iter) {

            view = qobject_cast<FrameView*> (*iter);
            if (view != 0 && (int) view->winId() == containerId) {
                view->handleEmbed(msg->getSrcId(), clientWId);
                break;
            } else {
                // close it, but we need to fake that this tab is its container
                sendSysCall(MSG_UPDATA_CONTAINER, QByteArray::number(msg->getSrcId()));
                sendSysCall(MSG_WEBAPP_CLOSE, QByteArray::number(msg->getSrcId()));
            }
        }
    }

    //    qDebug() << "embedFrame leaving"; // WID: " << m_iframewid.size() << " pending Widgets " << m_iframewidget.size();
}

void WebView::dispatchUIMsg(Message* msg)
{
    int progress;
    bool ok;
    QString link;
    QString title;
    QString textContent;
    QString text;
    QRect geom;
    bool visible;
    QUrl url;
    bool back;
    
    QByteArray data((char *)msg->getMsgData(), msg->getDataLen());
    QDataStream stream(data);
    QVariant var;
    if (data.size() > 0)
        stream >> var;
    
    switch (msg->getMsgValue()) {
    case MSG_loadStarted:
        m_progress = 0;
        emit loadStarted();
        break;
    case MSG_loadProgress:
        progress = var.toInt();
        m_progress = progress;
        if (progress == 100)
            m_progress = 0;
        emit loadProgress(progress);
        break;
    case MSG_loadFinished:
        ok = var.toBool();
        m_progress = 0;
        emit loadFinished(ok);
        break;
    case MSG_linkHovered:
        link = var.toString();
        emit linkHovered(link, title, textContent);
        break;
    case MSG_statusBarMessage:
        text = var.toString();
        emit statusBarMessage(text);
        break;
    case MSG_geometryChangeRequested:
        geom = var.toRect();
        emit geometryChangeRequested(geom);
        break;
    case MSG_windowCloseRequested:
        emit windowCloseRequested();
        break;
    case MSG_toolBarVisibilityChangeRequested:
        visible = var.toBool();
        emit toolBarVisibilityChangeRequested(visible);
        break;
    case MSG_statusBarVisibilityChangeRequested:
        visible = var.toBool();
        emit statusBarVisibilityChangeRequested(visible);
        break;
    case MSG_menuBarVisibilityChangeRequested:
        visible = var.toBool();
        emit menuBarVisibilityChangeRequested(visible);
        break;
    case MSG_titleChanged:
        title = var.toString();
        m_title = title;
        m_history->setCurrentItemTitle(title);
        emit titleChanged(title);
        break;
    case MSG_iconChanged:
        emit iconChanged();
        break;
    case MSG_urlChanged:
        url = var.toUrl();
        m_url = url;
        emit urlChanged(url);
        break;
    case MSG_addHistoryItem:
        link = var.toString();
        navToNewUrl(link);
        break;
    case MSG_navBackOrForward:
        back = var.toBool();
        m_history->ajustCurrentItem(back);
        updateAction(Forward);
        updateAction(Back);
        break;
    default:
        break;
    }
}


const QUrl WebView::url() const
{
    if (!m_url.isEmpty())
        return m_url;
    return m_initialUrl;
}

const QString WebView::title() const
{
    return m_title;
}

const QString WebView::lastStatusBarText() const
{
    return m_statusBarText;
}

int WebView::progress() const
{
    return m_progress;
}

void WebView::loadUrl(const QUrl &url, const QString &title, const bool& navigate)
{
    if (!navigate) {
        HistoryItem item(url.toString(), title);
        m_history->addItem(item);
    }
    updateAction(Back);
    updateAction(Forward);
    /*if (url.scheme() == QLatin1String("javascript")) {
        QString scriptSource = url.toString().mid(11);
        QVariant result = page()->mainFrame()->evaluateJavaScript(scriptSource);
        if (result.canConvert(QVariant::String)) {
            QString newHtml = result.toString();
            setHtml(newHtml);
        }
        return;
        }*/
    m_initialUrl = url;
    m_url.clear();
    if (title.isEmpty())
        emit titleChanged(tr("Loading..."));
    else
        emit titleChanged(title);
    if (m_clientId != 0) {
        sendWebAppMsg(MSG_WEBAPP_LOAD_URL, url.toString().toLatin1());
    } else {
        sendSysCall(MSG_NEW_URL, url.toString().toLatin1());
    }
}


void WebView::sendSysCall(int type, const QByteArray& content)
{
    Message msg;
    msg.setDstId(KERNEL_ID);
    msg.setMsgType(type);
    msg.setMsgValue(m_containerId);
    msg.setData(content.constData(), content.size());
    msg.writeMessage();
}
    
void WebView::sendWebAppMsg(int type, const QByteArray& content)
{
    if (m_clientId != 0) {
        Message msg;
        msg.setDstId(m_clientId);
        msg.setMsgType(MSG_WEBAPP_MSG);
        msg.setMsgValue(type);
        msg.setData(content.constData(), content.size());
        msg.writeMessage();
    }
}

void WebView::closeFrames()
{
    // clear unembeded client first
    while (m_iframeclientids.size() > 0) {
        int unembedded = m_iframeclientids.takeFirst();
        // close it, but we need to fake that this tab is its container
        sendSysCall(MSG_UPDATA_CONTAINER, QByteArray::number(unembedded));
        sendSysCall(MSG_WEBAPP_CLOSE, QByteArray::number(unembedded));
    }
    // close the iframes
    if(m_iframes.isEmpty())
        return;

    while (m_iframes.size() > 0) {
        FrameView* fr = qobject_cast<FrameView*> (m_iframes.takeFirst());
        if (fr != 0) {
            QWidget* parent = fr->parentWidget();
            fr->deleteLater();
            parent->deleteLater();
        }
    }
}

i32 WebView::ownId() const
{
    return m_containerId;
}

i32 WebView::clientId() const 
{
    return m_clientId;
}

QAction* WebView::action(WebAction action)
{
    if (action < 0 || action >= WebActionCount)
        return 0;
    if (m_actions[action])
        return m_actions[action];

    QString text;
    QIcon icon;
    bool checkable = false;

    switch (action) {
    case Stop:
        text = QString::fromAscii("Stop");
        icon = style()->standardIcon(QStyle::SP_BrowserStop);
        break;
    case Reload:
        text = QString::fromAscii("Reload");
        icon = style()->standardIcon(QStyle::SP_BrowserReload);
        break;
    case Back:
        text = QString::fromAscii("Back");
        icon = style()->standardIcon(QStyle::SP_ArrowBack);
        break;
    case Forward:
        text = QString::fromAscii("Forward");
        icon = style()->standardIcon(QStyle::SP_ArrowForward);
        break;
    default:
        text = QString::fromAscii("Not Impl");
        break;
    }
    QAction* a = new QAction(this);
    a->setText(text);
    a->setData(action);
    a->setCheckable(checkable);
    a->setIcon(icon);

    connect(a, SIGNAL(triggered(bool)),
            this, SLOT(actionTriggered(bool)));

    m_actions[action] = a;
    updateAction(action);
    return a;            
}

void WebView::actionTriggered(bool checked)
{
    Q_UNUSED(checked);
    QAction* a = qobject_cast<QAction*> (sender());
    if (!a)
        return;
    WebAction action = static_cast<WebAction>(a->data().toInt());

    switch (action) {
    case Stop:
        //stop();
        break;
    case Reload:
        reload();
        break;
    case Back:
        m_history->back();
        break;
    case Forward:
        m_history->forward();
        break;
    default:
        break;
    }
}

void WebView::updateAction(WebAction action)
{
    QAction* a = m_actions[action];
    if (!a)
        return;

    bool enabled = a->isEnabled();
    switch (action) {
    case Stop:
        qDebug() << "Stop!";
        break;
    case Reload:
        qDebug() << "Reload!";
        break;
    case Back:
        enabled = m_history->canGoBack();
        break;
    case Forward:
        enabled = m_history->canGoForward();
        break;
    default:
        break;
    }
    a->setEnabled(enabled);
}

void WebView::navToNewUrl(const QString& url)
{
    m_history->addItem(HistoryItem(url));
    updateAction(Forward);
    updateAction(Back);
}

void WebView::reload()
{
    qDebug() << "Reload!";
}

void WebView::print()
{
    qDebug() << "Print!";
}

void WebView::resetZoom()
{
    qDebug() << "resetZoom!";
}

void WebView::zoomIn()
{
    qDebug() << "zoomIn!";
}

void WebView::zoomOut()
{
    qDebug() << "zoomOut!";
}

