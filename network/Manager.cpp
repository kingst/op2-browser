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
#include "Manager.h"
#include "Message.h"
#include "Network.h"
#include "Cookie.h"
#include <QBuffer>
#include <QDataStream>
#include <QVariant>
#include <QCoreApplication>
#include <QDebug>
#include <QStringList>

#if QT_VERSION >= 0x040500
#include <QNetworkDiskCache>
#include <QDesktopServices>
#endif

Manager* Manager::_instance = NULL;

Manager* Manager::instance()
{
    if (_instance == NULL)
        _instance = new Manager();
    return _instance;
}

Manager::Manager()
    : QObject(0)
    , m_reqId(0)
    , m_waitForReply(false)
{
    //Q_UNUSED(idToChar);
    m_manager = new QNetworkAccessManager();
    m_stdin = new QSocketNotifier(Message::getReadFd(), QSocketNotifier::Read, NULL);
    connect(m_stdin, SIGNAL(activated(int)),
            this, SLOT(activated(int)));
    m_manager->setCookieJar(CookieJar::instance());

#if QT_VERSION >= 0x040500
    QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
    QString location = QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
    diskCache->setCacheDirectory(location);
    diskCache->setMaximumCacheSize(50*1024*1024);
    m_manager->setCache(diskCache);
#endif
}

Manager::~Manager()
{
    m_stdin->deleteLater();
    m_manager->deleteLater();
    _instance = NULL;
}

QNetworkAccessManager* Manager::networkAccessManager()
{
    return m_manager;
}

void Manager::activated(int fd)
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
            if (m_waitForReply && msg->getMsgType() != MSG_COOKIE_GET_RETURN) {
                m_msgQueue.push_back(msg);
                msg = new Message(); // since we store it, we need a new one
                continue;
            }
            dispatchMsg(msg);
        }
        delete msg;
    }
}


void Manager::dispatchMsg(Message* msg)
{
    //qDebug() << "Network Get Msg:" << msg->getMsgType();
    if (msg->getMsgType() == MSG_FETCH_URL) {
        m_requestQueue.append(new NetworkRequest(msg));
    } else if (msg->getMsgType() == MSG_COOKIE_GET_RETURN) {
        handleCookieMap((char*) msg->getMsgData(), msg->getDataLen(),
                        msg->getMsgValue());
    } else if (msg->getMsgType() == MSG_FETCH_URL_ABORT) {
        for (int i = 0; i < m_requestQueue.count(); ++i) {
            if (m_requestQueue.at(i)->webappId() == msg->getSrcId()
                && m_requestQueue.at(i)->urlId() == msg->getMsgValue() ) {
                m_requestQueue.at(i)->abort();
                break;
            }
        }            
    } else {
        qDebug() << "Unhandled NETWORK message type:" << msg->getMsgType();
    }
}

void Manager::removeRequest(NetworkRequest* req)
{
    int idx = m_requestQueue.indexOf(req);
    if (idx != -1) {
        m_requestQueue.removeAt(idx);
    }
}


void Manager::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList,
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

    Message msg(NETWORK_ID, COOKIE_ID, 0, MSG_COOKIE_SET,
                0, bytes.constData(), bytes.size());
    msg.writeMessage();
}

QList<QNetworkCookie> Manager::cookiesForUrl(const QUrl& url)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadWrite);
    QDataStream out(&buffer);
    QVariant var;
    int reqId = m_reqId++;

    var = QVariant(url);
    out << var;

    Message msg(NETWORK_ID, COOKIE_ID, 0, MSG_COOKIE_GET,
                reqId, bytes.constData(), bytes.size());
    msg.writeMessage();

    // wait for reply
    Q_ASSERT(m_waitForReply == false);
    if (m_waitForReply != false) {
        qDebug() << "Warning:" << "multiple waits";
    }
    m_waitForReply = true;

    // process pending STDIN_FILENO first
    activated(Message::getReadFd());
    while (m_cookieMap.find(reqId) == m_cookieMap.end()) {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }
    Q_ASSERT(m_waitForReply == true);
    m_waitForReply = false;

    // schedule a call to process queued messages
    QMetaObject::invokeMethod(this, "activated", Qt::QueuedConnection,
                              QGenericReturnArgument(), Q_ARG(int, -1));

    return QNetworkCookie::parseCookies(m_cookieMap.take(reqId).toLatin1());
}

void Manager::handleCookieMap(const char* data, int len, int reqId)
{
    QByteArray bytes(data, len);
    QDataStream in(bytes);
    QVariant var;

    in >> var;
    QString cookieStr = var.toString();
    m_cookieMap[reqId] = cookieStr;
}
