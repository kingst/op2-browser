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
#ifndef MANAGER_H
#define MANAGER_H

#include <QNetworkAccessManager>
#include <QSocketNotifier>
#include <QNetworkCookieJar>
#include <QMap>

class Message;
class NetworkRequest;

class Manager : public QObject
{
    Q_OBJECT
public:
    static Manager* instance();
    QNetworkAccessManager* networkAccessManager();
    ~Manager();

    void setCookiesFromUrl(const QList<QNetworkCookie> &cookieList,
			   const QUrl& url);
    QList<QNetworkCookie> cookiesForUrl(const QUrl& url);

    void removeRequest(NetworkRequest* req);
    
private:
    Manager();

    void dispatchMsg(Message* msg);

    QNetworkAccessManager* m_manager;
    QSocketNotifier* m_stdin;

    QMap<int, QString> m_cookieMap;
    int m_reqId;
    void handleCookieMap(const char* data, int len, int reqId);
    
    QList<Message*> m_msgQueue;
    bool m_waitForReply;

    QList<NetworkRequest*> m_requestQueue;

    static Manager* _instance;

private slots:
    void activated(int fd);
};

#endif
