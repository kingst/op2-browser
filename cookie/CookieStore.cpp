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
#include "CookieStore.h"
#include <QDateTime>
#include <QUrl>

#include "../kernel/kernel.h"

CookieStore* CookieStore::_instance = NULL;
QString CookieStore::_dbName = "cookies.db";

inline static bool shorterPaths(const QNetworkCookie &c1, const QNetworkCookie &c2)
{
    return c2.path().length() < c1.path().length();
}


CookieStore* CookieStore::instance()
{
    if (_instance == NULL) {
        _instance = new CookieStore(_dbName);
    }
    return _instance;
}

void CookieStore::release()
{
    if (_instance != NULL) {
        delete _instance;
        _instance = NULL;
    }
}

CookieStore::CookieStore(const QString& dbName)
    : m_dbName(dbName)
    , m_db(NULL)
{
    if (!init()) {
        exit(1);
    }
}

CookieStore::~CookieStore()
{
    sqlite3_close(m_db);
}

bool CookieStore::init()
{
    int rc;

    rc = sqlite3_open(m_dbName.toAscii().data(), &m_db);
    if (rc != SQLITE_OK) {
        qDebug() << "Cannot open database: " << sqlite3_errmsg(m_db);
        m_db = NULL;
        return false;
    }

    bool qr = false;

    qr = query("create table if not exists cookies (domain text, path text, name text, expiration text, content text, primary key (domain, path, name) on conflict replace)");
    qr = qr && query("PRAGMA synchronous=OFF");

    return qr;
}

inline static int sqliteCallBack(void* notUsed, int argc, char** argv, char** azColName) {
    Q_UNUSED(notUsed);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    Q_UNUSED(azColName);
    return 0;
}

bool CookieStore::query(const QString& stmt)
{
    char *zErrMsg = 0;
    int rc;

    if (m_db == NULL)
        return false;

    rc = sqlite3_exec(m_db, stmt.toLatin1().constData(), sqliteCallBack, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        qDebug() << "SQL error: " << zErrMsg;
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

bool CookieStore::insertCookie(const QNetworkCookie& cookie)
{
    sqlite3_stmt* stmt;
    int rc;
    
    QByteArray domain = cookie.domain().toLatin1();
    QByteArray path = cookie.path().toLatin1();
    QByteArray name = cookie.name();
    QByteArray expiration;
    if (cookie.isSessionCookie()) {
        expiration = "0";
    } else {
        expiration = cookie.expirationDate().toString().toLatin1();
    }
    QByteArray content = cookie.toRawForm();

    rc = sqlite3_prepare(m_db,
                         "insert into cookies (domain, path, name, expiration, content) values (?, ?, ?, ?, ?)",
                         -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        qDebug() << "insertCookie prepare fail:" << rc;
        return false;
    }

    QByteArray text[5] = {domain, path, name, expiration, content};
    
    for (int i = 0; i < 5; ++i) {
        rc = sqlite3_bind_text(stmt, i + 1, text[i].constData(), text[i].size(), SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            qDebug() << "insertCookie bind fail:" << i;
            sqlite3_finalize(stmt);
            return false;
        }
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        qDebug() << "insertCookie step (execute) fail";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool CookieStore::deleteCookie(const QNetworkCookie& cookie)
{
    sqlite3_stmt *stmt;
    int rc;

    QByteArray domain = cookie.domain().toLatin1();
    QByteArray path = cookie.path().toLatin1();
    QByteArray name = cookie.name();

    QByteArray text[3] = {domain, path, name};

    rc = sqlite3_prepare(m_db,
                         "delete from cookies where (domain = ? and path = ? and name = ?)",
                         -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        qDebug() << "deleteCookie prepare fail.";
        return false;
    }

    for (int i = 0; i < 3; ++i) {
        rc = sqlite3_bind_text(stmt, i + 1, text[i].constData(), text[i].size(), SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            qDebug() << "deleteCookie bind fail.";
            sqlite3_finalize(stmt);
            return false;
        }
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        qDebug() << "deleteCookie step (execute) fail";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

QList<QNetworkCookie> CookieStore::queryCookies(const QString& domain)
{
    QList<QNetworkCookie> cookies;

    sqlite3_stmt* stmt;
    int rc;

    rc = sqlite3_prepare(m_db,
                         "select content from cookies where (domain = ?)",
                         -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        qDebug() << "queryCookies prepare fail." << rc;
        return cookies;
    }

    rc = sqlite3_bind_text(stmt, 1, domain.toLatin1().constData(), domain.size(), SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        qDebug() << "queryCookies bind fail.";
        sqlite3_finalize(stmt);
        return cookies;
    }

    rc = sqlite3_step(stmt);
    while (rc == SQLITE_ROW) {
        const char* raw = (const char*) sqlite3_column_text(stmt, 0);
        QByteArray cookieStr(raw);
        QList<QNetworkCookie> cookie = QNetworkCookie::parseCookies(cookieStr);
        if (!cookie.isEmpty())
            cookies.push_back(cookie.first());
        rc = sqlite3_step(stmt);
    }
    if (rc != SQLITE_DONE) {
        qDebug() << "queryCookies step (execute) fail.";
    }
    sqlite3_finalize(stmt);
    return cookies;
}

QList<QNetworkCookie> CookieStore::cookiesForUrl(const QUrl &url)
{
    QList<QNetworkCookie> cookies;

    QStringList domains = qualifiedDomains(url);

    foreach (QString domain, domains) {
        cookies += queryCookies(domain);
    }

    if (cookies.isEmpty()) {
        return cookies;
    }

    QDateTime now = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);
    const QString path = urlPath(url);
    const bool isSecure = url.scheme().toLower() == QLatin1String("https");
    QList<QNetworkCookie>::iterator i = cookies.begin();
    for ( ; i != cookies.end(); ) {
    	if (!checkPath(*i, url)) {
            i = cookies.erase(i);
            continue;
        }
        if (!isSecure && i->isSecure()) {
            i = cookies.erase(i);
            continue;
        }
        if (!i->isSessionCookie() && now > i->expirationDate()) {
            // remove expired cookie from database
            deleteCookie(*i);
            i = cookies.erase(i);
            continue;
        }
        ++i;
    }

    qSort(cookies.begin(), cookies.end(), shorterPaths);

    
    /*QStringList cookieStrList;
      foreach (QNetworkCookie cookie, cookies) {
      cookieStrList.push_back(cookie.toRawForm());
      }
      qDebug() << "\nReturn:\n" << cookieStrList.join("\"\n \"").toLatin1() << "\nfor Url:" << url.toString() << "\n";
    */
    return cookies;
}

bool CookieStore::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)
{
    QDateTime now = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);
    bool changed = false;
    QString defaultPath = url.path();
    defaultPath = defaultPath.mid(0, defaultPath.lastIndexOf(QLatin1Char('/')) + 1);

    foreach (QNetworkCookie cookie, cookieList) {

        if (cookie.path().isEmpty()) {
            // if the cookie doesn't have path, assign one
            cookie.setPath(defaultPath);
        } else if (!checkPath(cookie, url)) {
            continue;
        }

        if (cookie.domain().isEmpty()) {
            // if the cookie doesn't have domain, assign one
            cookie.setDomain(url.host().toLower());
        } else {
            /*if (cookie.domain().startsWith(QLatin1Char('.')) && cookie.domain().toLower().mid(1) == url.host().toLower()) {
            // QT 4.5.2 add a prefix . to all cookie domain. so stupid
            // let remove it
            cookie.setDomain(url.host().toLower());
            }*/
            if (cookie.domain().toLower() != url.host().toLower()) {
                if (!cookie.domain().startsWith(QLatin1Char('.'))) {
                    // if the given domain doesn't start with '.', assign one
                    cookie.setDomain(QLatin1Char('.') + cookie.domain());
                }
                if (!checkDomain(cookie, url)) {
                    continue;
                }
            }
        }

        // TODO check port ....

        // remove existing ones
        deleteCookie(cookie);
	
        bool dead = !cookie.isSessionCookie() && now > cookie.expirationDate();
        if (dead)
            continue;

        changed = true;
        insertCookie(cookie);
    }

    QStringList cookieStrList;
    foreach (QNetworkCookie cookie, cookieList) {
        cookieStrList.push_back(cookie.toRawForm());
    }
    //qDebug() << "\nSet:\n" << cookieStrList.join("\"\n \"").toLatin1() << "\nfor Url:" << url.toString() << "\n";

    
    return changed;
}


QStringList CookieStore::qualifiedDomains(const QUrl& url)
{
    QString domain = url.host();
    
    QStringList domains;
    domains.push_back(domain);
    
    domains.push_back(QLatin1Char('.') + domain);   // This doesn't obey RFC, but ...

    QStringList domainParts = domain.toLower().split(QLatin1Char('.'));
    domainParts.removeFirst();
    while (domainParts.size() >= 2) {
        domains.push_back(QLatin1String(".") + domainParts.join(QLatin1String(".")));
        domainParts.removeFirst();
    }
    return domains;
}

QString CookieStore::urlPath(const QUrl& url) const
{
    QString path = url.path();
    //    path = path.mid(0, path.lastIndexOf(QLatin1Char('/')) + 1);
    if (!path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
    }
    return path.toLower();
}

bool CookieStore::checkPath(const QNetworkCookie& cookie, const QUrl& url)
{
    QString cookiePath = cookie.path().toLower();
    QString hostPath = urlPath(url);
    if (!cookiePath.endsWith(QLatin1Char('/'))) {
        cookiePath += QLatin1Char('/');
    }
    //    qDebug() << "cookiePath:" << cookiePath << "<-> hostPath:" << hostPath;
    return hostPath.startsWith(cookiePath);
}

bool CookieStore::checkDomain(const QNetworkCookie& cookie, const QUrl& url)
{
    QString cookieDomain = cookie.domain().toLower();
    QString hostDomain = url.host().toLower();

    if (cookieDomain == hostDomain)
        return true;

    // this doesn't obey RFC2109 4.3.2 (RFC2965 3.3.2)
    // but we have to do this to use the new Yahoo homepage...
    if (cookieDomain.startsWith(QLatin1Char('.')) && (cookieDomain.mid(1) == hostDomain))
        return true;
    
    QStringList urlParts = hostDomain.split(QLatin1Char('.'));
    urlParts.removeFirst();

    // This doesn't obey RFC2109 4.3.2 (RFC2965 3.3.2)
    // but we have to do this to login ESPN ...
    QStringList cookieParts = cookieDomain.split(QLatin1Char('.'), QString::SkipEmptyParts);
    while (urlParts.count() > cookieParts.count()) {  
        urlParts.removeFirst();
    }
    
    QString matchDomain = QLatin1String(".") + urlParts.join(QLatin1String("."));
    if (cookieDomain == matchDomain)
        return true;
    
    return false;
}
