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
#ifndef OP_NETWORK_H
#define OP_NETWORK_H
#include <QNetworkAccessManager>
#include <QSocketNotifier>
#include <QList>
#include <QMap>
#include <QNetworkCookie>
#include <QDebug>

class Message;

namespace OPNET {
    extern int id;

    class OpNetworkReply;
    class OpNetworkAccessManager;

    class OpNetwork : public QObject {
        Q_OBJECT
	    
        static OpNetwork* _instance;

        OpNetwork();

    public:
        static OpNetwork* instance();
        QNetworkAccessManager* networkAccessManager();

        ~OpNetwork();
        inline int reqId() { return m_reqId++; }

        void setCookiesFromUrl(const QList<QNetworkCookie> &cookieList,
                               const QUrl& url);
        QList<QNetworkCookie> cookiesForUrl(const QUrl& url);

        int createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest& r,
                          QIODevice* outgoingData);

        void init(int winId);
        void sendUIMsg(int type, const QByteArray& content);
        void sendSysCall(int type, int value, const QByteArray& content);
        void sendDownloadRequest(int id, const QByteArray& data);
        QByteArray sendUploadRequest(const QByteArray& filename);
            
        inline bool isReplay() { return m_isReplay; }
        inline bool isReplica() { return m_isReplica; }
        inline QByteArray getName() { return m_webAppName; }

    signals:
        void newUrl(int winId, const QByteArray& content);
        void webAppMsg(int type, const QByteArray& content);
        void userAction(int type, const QByteArray& content);

    private:
        OpNetworkAccessManager* m_manager;
        QSocketNotifier* m_stdin;

        int m_reqId;
        QByteArray m_webAppName;
        bool m_isReplay;
        bool m_isReplica;

        QList<Message*> m_msgQueue;
        QMap<int, QString> m_cookieMap;
        bool m_waitForReply;

        void dispatchMsg(Message* msg);
	
        void handleCookieMap(int cookieId, const QByteArray& cookieData);
        void handleReturnUrl(int urlId, const QByteArray& urlData);
        void handleMetaData(int urlId, const QByteArray& metaData);
        void handleSetUrl(int winId, const QByteArray& urlData);
        void handleWebAppMsg(int type, const QByteArray& webAppMsgData);
        void handleUserAction(int type, const QByteArray& actionData); // for replay
        void handleSetPolicy(const QByteArray& policyData);
        void handleSetReplay(const QByteArray& nameData);

        private slots:
        void activated(int fd);
    };
}

#endif
