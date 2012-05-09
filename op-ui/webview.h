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
#ifndef WEBVIEW_H
#define WEBVIEW_H


#include <QX11EmbedContainer>
#include <QUrl>
#include <QMap>

QT_BEGIN_NAMESPACE
class QUrl;
class QString;
class QAction;
QT_END_NAMESPACE

class Message;
class History;

class WebView : public QX11EmbedContainer
{
    Q_OBJECT

public:
    enum WebAction {
        Undo,
        Redo,
        Cut,
        Copy,
        Paste,
        Stop,
        Reload,
        Back,
        Forward,

        WebActionCount
    };

    enum FindFlag {
        FindBackward = 1,
        FindCaseSensitively = 2,
        FindWrapsAroundDocument = 4
    };

    Q_DECLARE_FLAGS(FindFlags, FindFlag)

    
signals:

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

protected slots:
    void slotClientClosed();               
    void slotClientIsEmbeded();
    void actionTriggered(bool);
    void reEmbedClient();
    
public:
    WebView(QWidget* parent=0);
    ~WebView();
    void dispatchUIMsg(Message* msg);
    void handleEmbed(Message* msg);
    void embedFrame(Message* msg);
    void prepFrame(Message* msg);
    
    // dummys
    QObject* page() { return (QObject *)this; };
   
    //void webPage() {};
    bool findText(const QString& str, FindFlags flag) { Q_UNUSED(str);  Q_UNUSED(flag); return false; };
    bool isModified() { return false; }


    // real
    const QUrl url() const ;
    int progress() const ;
    const QString title() const ;
    const QString lastStatusBarText() const;
    QAction* action(WebView::WebAction action);
    void navToNewUrl(const QString& url);
    void reload();
    void print();
    void zoomIn();
    void resetZoom();
    void zoomOut();

    History* history() { return m_history; }
    
    void loadUrl(const QUrl &url, const QString &title = QString(), const bool& navigate = false);
    int ownId() const;
    int clientId() const;

    void closeFrames();

    void sendSysCall(int type, const QByteArray& content);
    void sendWebAppMsg(int type, const QByteArray& content);
private:
    int m_containerId;
    int m_clientId;
    int m_clientWId;
    int m_nextClientWId;
    int m_lastClientWId;
    bool m_embedding;
    
    QString m_title;
    int m_progress;
    QUrl m_url;
    QString m_statusBarText;
    QUrl m_initialUrl;
    History* m_history;
    QList<QWidget *> m_iframes;
    QList<QString> m_iframenames;
    QList<int> m_iframewids;
    QList<int> m_iframeclientids;

    QAction* m_actions[WebView::WebActionCount];
    void updateAction(WebAction);
    void setClientIds(int clientId, int clientWId);
    
};

#endif
