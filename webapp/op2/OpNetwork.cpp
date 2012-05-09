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
#include "OpNetwork.h"
#include "OpCookieJar.h"
#include "OpNetworkAccessManager.h"
#include "../../kernel/Message.h"

#include <QBuffer>
#include <QDataStream>
#include <QVariant>
#include <QCoreApplication>
#include <QDebug>
#include <QStringList>
#include <QUrl>
#include <QNetworkRequest>

using namespace OPNET;

int OPNET::id = 0;

OpNetwork* OpNetwork::_instance = NULL;

OpNetwork* OpNetwork::instance()
{
    if (_instance == NULL)
        _instance = new OpNetwork();
    return _instance;
}

OpNetwork::OpNetwork()
    : QObject(0)
    , m_reqId(0)
    , m_isReplay(false)
    , m_waitForReply(false)
{
    m_manager = new OpNetworkAccessManager();
    m_stdin = new QSocketNotifier(Message::getReadFd(), QSocketNotifier::Read, NULL);
    connect(m_stdin, SIGNAL(activated(int)),
            this, SLOT(activated(int)));
    m_manager->setCookieJar(OpCookieJar::instance());
}


OpNetwork::~OpNetwork()
{
    m_stdin->deleteLater();
    m_manager->deleteLater();
    _instance = NULL;
}

QNetworkAccessManager* OpNetwork::networkAccessManager()
{
    return m_manager;
}

void OpNetwork::activated(int fd)
{
    // process queued msg first
    if (!m_waitForReply) {
        while (!m_msgQueue.isEmpty()) {
            Message* msg = m_msgQueue.takeFirst(); // get and remove
            dispatchMsg(msg);
            delete msg;
        }
    }
    if (fd == Message::getReadFd()) {
        Message* msg = new Message();
        while (msg->hasDataAvailable()) {
            msg->readMessage();
            if (isReplay() && msg->getMsgId() > 0) {
                // we have a problem here, the msgId is 64 bit, but value is 32 bit
                sendSysCall(MSG_ACK_REPLAY_MSG, msg->getMsgId(), QByteArray());
            }
            if (m_waitForReply &&
                (msg->getMsgType() != MSG_DOM_COOKIE_GET_RETURN
                 && msg->getMsgType() != MSG_WEBAPP_MSG)) {
                m_msgQueue.push_back(msg);
                msg = new Message(); // since we store it, we need a new one
                continue;
            }
            dispatchMsg(msg);
        }
        delete msg;
    }
}


void OpNetwork::dispatchMsg(Message* msg)
{
    id = msg->getDstId();
    QByteArray data((char*) msg->getMsgData(), msg->getDataLen());
    int value = msg->getMsgValue();
    int type = msg->getMsgType();

    switch (type) {
    case MSG_RETURN_URL:
        handleReturnUrl(value, data);
        break;
    case MSG_RETURN_URL_METADATA:
        handleMetaData(value, data);
        break;
    case MSG_SET_URL:
        handleSetUrl(value, data);
        break;
    case MSG_DOM_COOKIE_GET_RETURN:
        handleCookieMap(value, data);
        break;
    case MSG_WEBAPP_MSG:
        handleWebAppMsg(value, data);
        break;
    case MSG_SET_POLICY:
        handleSetPolicy(data);
        break;
    case MSG_LOG_USER_INPUT:
        handleUserAction(value, data);
        break;
    case MSG_SET_REPLAY_MODE:
        handleSetReplay(data);
        break;
    case MSG_READ_FILE_RETURN:
        break;
    default:
        qDebug() << "Unhandled OPNET message type:" << type;
        break;
    }
}


void OpNetwork::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList,
                                  const QUrl& url)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadWrite);
    QDataStream out(&buffer);
    QVariant var;

    var = QVariant(url);
    out << var;

    QStringList cookieStrList;
    foreach (QNetworkCookie cookie, cookieList) {
        cookieStrList.push_back(cookie.toRawForm());
    }
    QString cookieStr = cookieStrList.join(QLatin1String(", "));

    var = QVariant(cookieStr);
    out << var;

    Message msg(-1, COOKIE_ID, 0, MSG_DOM_COOKIE_SET,
                0, bytes.constData(), bytes.size());
    msg.writeMessage();
}

QList<QNetworkCookie> OpNetwork::cookiesForUrl(const QUrl& url)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadWrite);
    QDataStream out(&buffer);
    QVariant var;
    int reqId = this->reqId();
    QByteArray cookieStr;

    var = QVariant(url);
    out << var;

    Message msg(-1, COOKIE_ID, 0, MSG_DOM_COOKIE_GET,
                reqId, bytes.constData(), bytes.size());
    msg.writeMessage();

    // wait for reply
    //Q_ASSERT(m_waitForReply == false);
    if (m_waitForReply != false) {
        qDebug() << "Warning:" << "multiple waits";
    }
    m_waitForReply = true;

    // process pending STDIN_FILENO first
    activated(Message::getReadFd());
    while (m_cookieMap.find(reqId) == m_cookieMap.end()) {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }
    cookieStr = m_cookieMap.take(reqId).toLatin1();
    
    Q_ASSERT(m_waitForReply == true);
    if (m_cookieMap.isEmpty()) {
        m_waitForReply = false;
    }

    // schedule a call to process queued messages
    QMetaObject::invokeMethod(this, "activated", Qt::QueuedConnection,
                              QGenericReturnArgument(), Q_ARG(int, -1));

    return QNetworkCookie::parseCookies(cookieStr);
}

void OpNetwork::handleCookieMap(int reqId, const QByteArray& cookieData)
{
    QDataStream in(cookieData);
    QVariant var;

    in >> var;
    QString cookieStr = var.toString();
    m_cookieMap[reqId] = cookieStr;
}

void OpNetwork::handleReturnUrl(int urlId, const QByteArray& urlData)
{
    m_manager->handleData(urlId, urlData);
}

void OpNetwork::handleMetaData(int urlId, const QByteArray& metaData)
{
    m_manager->handleMetaData(urlId, metaData);
}

void OpNetwork::handleSetUrl(int winId, const QByteArray& urlData)
{
    emit newUrl(winId, urlData);

    if (m_webAppName.size() == 0)
        m_webAppName = QUrl(urlData).host().toLatin1();
}

void OpNetwork::handleWebAppMsg(int type, const QByteArray& webAppMsgData)
{
    emit webAppMsg(type, webAppMsgData);
}

void OpNetwork::handleUserAction(int type, const QByteArray& actionData) // for replay
{
    emit userAction(type, actionData);
}

void OpNetwork::handleSetPolicy(const QByteArray& policyData)
{
    Q_UNUSED(policyData);
    // FIXME
}

void OpNetwork::handleSetReplay(const QByteArray& nameData)
{
    m_isReplay = true;
    m_webAppName = nameData;
}

int OpNetwork::createRequest(QNetworkAccessManager::Operation op,
                             const QNetworkRequest& req,
                             QIODevice* outgoingData)
{
    int urlId = this->reqId();

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadWrite);
    QDataStream out(&buffer);
    QVariant var;

    var = QVariant((int)op);
    out << var;

    var = QUrl(req.url());
    out << var;
    putRequest(req.url().toString());
    
    //QList<QVariant> attributes;
    
    QMap<QString, QVariant> headers;
    foreach (QByteArray headerName, req.rawHeaderList()) {
        headers[headerName] = QVariant(req.rawHeader(headerName));
    }
    var = QVariant(headers);
    out << var;

    if (outgoingData != NULL) {
        //	outgoingData->open(QIODevice::ReadOnly);
        var = QVariant(outgoingData->readAll());
    }
    else {
       	var = QVariant(QByteArray());
    }
    out << var;
    
    Message msg(0, NETWORK_ID, 0, MSG_FETCH_URL, urlId, bytes.constData(), bytes.size());
    msg.writeMessage();
    return urlId;
}

void OpNetwork::abortRequest(int urlId)
{
    QByteArray data;
    Message msg(0, NETWORK_ID, 0, MSG_FETCH_URL_ABORT, urlId, data.constData(), data.size());
    msg.writeMessage();
}

void OpNetwork::init(int winId)
{
    Message msg(0, KERNEL_ID, 0, MSG_NEW_WEBAPP, winId, "", 0);
    msg.writeMessage();
}

void OpNetwork::sendUIMsg(int type, const QByteArray& content)
{
    Message msg(0, UI_ID, 0, MSG_UI_MSG, type, content.constData(), content.size());
    msg.writeMessage();
}

void OpNetwork::sendSysCall(int type, int value, const QByteArray& content)
{
    Message msg(0, KERNEL_ID, 0, type, value, content.constData(), content.size());
    msg.writeMessage();
}

void OpNetwork::sendDownloadRequest(int id, const QByteArray& data)
{
    Message msg(0, STORAGE_ID, 0, MSG_WRITE_FILE, id, data.constData(), data.size());
    msg.writeMessage();
}

QByteArray OpNetwork::sendUploadRequest(const QByteArray& filename)
{
    return QByteArray();
    // TODO
}
