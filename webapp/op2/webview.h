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
#ifndef OP2_WEBVIEW_H
#define OP2_WEBVIEW_H

#define IS_CRAWLER ((getenv("OP_CRAWLER") != NULL) && (strcmp(getenv("OP_CRAWLER"),"YES") == 0))

#include <qwebview.h>
#include <QNetworkRequest>

QT_BEGIN_NAMESPACE
class QMouseEvent;
class QNetworkReply;
class QVariant;
QT_END_NAMESPACE

class WebPage;
class Message;

QByteArray getByteArray(const QVariant& var);

class WebView : public QWebView
{
        Q_OBJECT

public:
    enum OpenType { ThisWebApp = 0, NewWebApp,  NewTabNoSelect, NewTabSelect, NewWindow, NewSubFrame };
        
public:
    WebView(QWidget *parent = 0);
    WebPage *webPage() const { return m_page; }
    void loadPolicy(const QNetworkRequest &request, const QByteArray& body, OpenType type);

private:
//    void loadUrl(const QUrl &url, const QString &title = QString());
    void loadUrl(const QNetworkRequest &request, QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation, const QByteArray &body = QByteArray());
    void load(const QNetworkRequest &request, QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation, const QByteArray &body = QByteArray());
    void load(QUrl &url);
    void setHtml(const QString &html, const QUrl &baseUrl = QUrl());
    void setContent ( const QByteArray & data, const QString & mimeType = QString(), const QUrl & baseUrl = QUrl() );
    // help fucns
    static  QByteArray encodedNewUrlMsg(const QNetworkRequest& request, const QByteArray& body);
    static  void decodedNewUrlMsg(const QByteArray& msg, QNetworkRequest& request, QByteArray& body);

    void snapshot(QString path);
    
public:
    QUrl url() const;

    QString lastStatusBarText() const;
    inline int progress() const { return m_progress; }
    inline bool loaded() const { return m_loaded; }
    
    
public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    void wheelEvent(QWheelEvent *event);
    void resizeEvent(QResizeEvent *event);
    void closeEvent(QCloseEvent *event);

private slots:
    void setProgress(int progress);
    void loadFinished();
    void setStatusBarText(const QString &string);
    void downloadRequested(const QNetworkRequest &request);
    void openLink();
    void openLinkInNewTab();
    void openLinkInNewWindow();
    void downloadLinkToDisk();
    void copyLinkToClipboard();
    void openImageInNewTab();
    void openImageInNewWindow();
    void downloadImageToDisk();
    void copyImageToClipboard();
    void copyImageLocationToClipboard();
    void bookmarkLink();

    void setUrl(int uid, const QByteArray& content);
    void slotWebAppMsg(int type, const QByteArray& content);
        

    void loadStarted();
    void loadProgress(int progress);
    void loadFinished(bool ok);

    void linkHovered(const QString &link, const QString &title, const QString &textContent);
    void statusBarMessage(const QString& text);
    void geometryChangeRequested(const QRect& geom);

    void windowCloseRequested();

    void toolBarVisibilityChangeRequested(bool visible);
    void statusBarVisibilityChangeRequested(bool visible);
    void menuBarVisibilityChangeRequested(bool visible);


    void titleChanged(const QString& title);
    void iconChanged();
    void urlChanged(const QUrl &url);
    
    
private:
    QString m_statusBarText;
    QUrl m_initialUrl;
    QNetworkRequest m_request;
    QByteArray m_postData;
    int m_progress;
    int m_currentZoom;
    QList<int> m_zoomLevels;
    WebPage *m_page;
    bool m_loaded;
    bool m_finished;

public:
    bool m_userAction;
};

#endif

