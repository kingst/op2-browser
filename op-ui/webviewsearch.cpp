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
/*
 * Copyright 2008 Benjamin C. Meyer <ben@meyerhome.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "webviewsearch.h"

#include <qevent.h>
#include <qshortcut.h>
#include <qtimeline.h>

#include <qdebug.h>

WebViewSearch::WebViewSearch(QWidget *parent)
    : QWidget(parent)
    , m_widget(0)
    , m_webView(0)
    , m_timeLine(new QTimeLine(150, this))
{
    initializeSearchWidget();

    // we start off hidden
    setMaximumHeight(0);
    m_widget->setGeometry(0, -1 * m_widget->height(),
                          m_widget->width(), m_widget->height());
    hide();

    connect(m_timeLine, SIGNAL(frameChanged(int)),
            this, SLOT(frameChanged(int)));

    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(animateHide()));
}

void WebViewSearch::initializeSearchWidget()
{
    m_widget = new QWidget(this);
    m_widget->setContentsMargins(0, 0, 0, 0);
    ui.setupUi(m_widget);
    ui.previousButton->setText(QChar(9664));
    ui.nextButton->setText(QChar(9654));
    ui.searchInfo->setText(QString());
    connect(ui.nextButton, SIGNAL(clicked()),
            this, SLOT(findNext()));
    connect(ui.previousButton, SIGNAL(clicked()),
            this, SLOT(findPrevious()));
    connect(ui.searchLineEdit, SIGNAL(returnPressed()),
            this, SLOT(findNext()));
    connect(ui.doneButton, SIGNAL(clicked()),
            this, SLOT(animateHide()));

    setMinimumWidth(m_widget->minimumWidth());
    setMaximumWidth(m_widget->maximumWidth());
    setMinimumHeight(m_widget->minimumHeight());
}

void WebViewSearch::setWebView(WebView *webView)
{
    m_webView = webView;
}

WebView *WebViewSearch::webView() const
{
    return m_webView;
}

void WebViewSearch::clear()
{
    ui.searchLineEdit->setText(QString());
}

void WebViewSearch::showFind()
{
    if (!isVisible()) {
        show();
        m_timeLine->setFrameRange(-1 * m_widget->height(), 0);
        m_timeLine->setDirection(QTimeLine::Forward);
        disconnect(m_timeLine, SIGNAL(finished()),
                   this, SLOT(hide()));
        m_timeLine->start();
    }
    ui.searchLineEdit->setFocus();
    ui.searchLineEdit->selectAll();
}

void WebViewSearch::findNext()
{
    find(WebView::FindWrapsAroundDocument);
}

void WebViewSearch::findPrevious()
{
    find(WebView::FindBackward | WebView::FindWrapsAroundDocument);
}

void WebViewSearch::find(WebView::FindFlags flags)
{
    QString searchString = ui.searchLineEdit->text();
    if (!m_webView || searchString.isEmpty())
        return;
    QString infoString;
    if (!m_webView->findText(searchString, flags))
        infoString = tr("Not Found");
    ui.searchInfo->setText(infoString);
}

void WebViewSearch::resizeEvent(QResizeEvent *event)
{
    if (event->size().width() != m_widget->width())
        m_widget->resize(event->size().width(), m_widget->height());
    QWidget::resizeEvent(event);
}

void WebViewSearch::animateHide()
{
    m_timeLine->setDirection(QTimeLine::Backward);
    m_timeLine->start();
    connect(m_timeLine, SIGNAL(finished()), this, SLOT(hide()));
}

void WebViewSearch::frameChanged(int frame)
{
    if (!m_widget)
        return;
    m_widget->move(0, frame);
    int height = qMax(0, m_widget->y() + m_widget->height());
    setMinimumHeight(height);
    setMaximumHeight(height);
}

WebViewWithSearch::WebViewWithSearch(QWidget *parent)
    : WebView(parent)
    , m_webView(this)
{
    //m_webView->setParent(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    m_webViewSearch = new WebViewSearch(this);
    layout->addWidget(m_webViewSearch);
    m_webViewSearch->setWebView(m_webView);
    //layout->addWidget(m_webView);
    setLayout(layout);
}
