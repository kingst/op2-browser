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
#include "jswindow.h"
#include "OpNetwork.h"
#include "webview.h"

#include <QNetworkRequest>
#include <QFileInfo>

JSWindow::JSWindow(QObject *parent)
    : QWebPage(parent)
    , m_view(0)
{
    // set manager to avoid uncontrolled network access
    // though it seems it won't request any url because
    // of the navigation logic below
    setNetworkAccessManager(OPNET::OpNetwork::instance()->networkAccessManager());
}

void JSWindow::setView(WebView* view)
{
    m_view = view;
}

bool JSWindow::acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request,
			     NavigationType type)
{
    Q_UNUSED(frame);
    Q_UNUSED(type);
    if (request.url() == QUrl::fromLocalFile(QFileInfo(QUrl().toLocalFile()).absoluteFilePath())) {
	// webkit open a empty url before navigating to the real one
	// this is copied from qwebframe ensureabsoluteurl()
    } else {
	// here we default to open a new seleted tab for the new window
	if (m_view)
	    m_view->loadPolicy(request, QByteArray(), WebView::NewTabSelect);
	deleteLater(); // FIXME if we want inter window js communication
    }
    return false;
}

QWebPage* JSWindow::createWindow(QWebPage::WebWindowType type)
{
    Q_UNUSED(type);
    qDebug() << "Warning: JS Window creates new window!";
    return 0;
}

#ifndef QT_NO_UITOOLS
QObject* JSWindow::createPlugin(const QString& classId, const QUrl& url,
		      const QStringList& paramNames, const QStringList& paramValues)
{
    Q_UNUSED(classId);
    Q_UNUSED(url);
    Q_UNUSED(paramNames);
    Q_UNUSED(paramValues);
    return 0;
}
#endif
