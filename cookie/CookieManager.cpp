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
#include "CookieManager.h"
#include "Message.h"
#include "CookieStore.h"
#include <QUrl>
#include <QDataStream>
#include <QBuffer>
#include <QVariant>
#include <QStringList>

CookieManager* CookieManager::_instance = NULL;

CookieManager* CookieManager::instance()
{
    //Q_UNUSED(idToChar);
    if (_instance == NULL)
	_instance = new CookieManager();
    return _instance;
}

CookieManager::CookieManager()
    : QObject(0)
{
    m_stdin = new QSocketNotifier(Message::getReadFd(), QSocketNotifier::Read, NULL);
    connect(m_stdin, SIGNAL(activated(int)),
            this, SLOT(activated(int)));
    m_store = CookieStore::instance();
}

CookieManager::~CookieManager()
{
    CookieStore::release();
    m_stdin->deleteLater();
}

void CookieManager::activated(int fd)
{
    if (fd == Message::getReadFd()) {
        m_stdin->setEnabled(false);
        Message msg;
        while (msg.hasDataAvailable()) {
            msg.readMessage();
            dispatchMsg(&msg);
        }
        m_stdin->setEnabled(true);
    }
}

void CookieManager::dispatchMsg(Message* msg)
{
    QUrl url;
    QList<QNetworkCookie> cookies;
    QByteArray data((char*) msg->getMsgData(), msg->getDataLen());
    QDataStream in(data);
    QVariant var;
    
    if (msg->getMsgType() == MSG_COOKIE_SET || msg->getMsgType() == MSG_DOM_COOKIE_SET) {
        in >> var;
        url = var.toUrl();
        in >> var;
        cookies = QNetworkCookie::parseCookies(var.toString().toLatin1());

        if (!cookies.isEmpty())
            m_store->setCookiesFromUrl(cookies, url);
	
    } else if (msg->getMsgType() == MSG_COOKIE_GET || msg->getMsgType() == MSG_DOM_COOKIE_GET) {
        in >> var;
        url = var.toUrl();
        cookies = m_store->cookiesForUrl(url);

        QStringList cookieList;
        foreach (QNetworkCookie cookie, cookies) {
            cookieList.push_back(cookie.toRawForm());
        }

        QString cookieStr = cookieList.join(QLatin1String(", "));
	
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::ReadWrite);
        QDataStream out(&buffer);
        var = QVariant(cookieStr);
        out << var;

        int retType;
        if (msg->getMsgType() == MSG_COOKIE_GET)
            retType = MSG_COOKIE_GET_RETURN;
        if (msg->getMsgType() == MSG_DOM_COOKIE_GET)
            retType = MSG_DOM_COOKIE_GET_RETURN;
        
        Message reply(COOKIE_ID, msg->getSrcId(), 0, retType,
                      msg->getMsgValue(), bytes.constData(), bytes.size());
        reply.writeMessage();
	
    } else {
        qDebug() << "Unhandled Message Type in Cookide Manager";
    }
    
    return;
}

