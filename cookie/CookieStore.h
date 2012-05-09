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
#ifndef COOKIE_STORE_H
#define COOKIE_STORE_H
#include <sqlite3.h>
#include <QDebug>
#include <QNetworkCookie>
#include <QStringList>

class CookieStore
{
public:
    static CookieStore* instance();
    static void release();
    ~CookieStore();

    QList<QNetworkCookie> cookiesForUrl(const QUrl &url);
    bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url);
    
private:
    CookieStore(const QString& dbName);

    bool init();
    bool query(const QString& stmt);
    bool insertCookie(const QNetworkCookie& cookie);
    bool deleteCookie(const QNetworkCookie& cookie);
    QList<QNetworkCookie> queryCookies(const QString& domain);

    QStringList qualifiedDomains(const QUrl& url);
    QString urlPath(const QUrl& url) const;
    bool checkPath(const QNetworkCookie& cookie, const QUrl& url);
    bool checkDomain(const QNetworkCookie& cookie, const QUrl& url);
    
    QString m_dbName;
    sqlite3* m_db;

    static CookieStore* _instance;
    static QString _dbName;
    static QString _dbNameReplica;
};

#endif
