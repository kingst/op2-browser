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

#include "locationbar.h"
#include "locationbar_p.h"

#include "browserapplication.h"
#include "searchlineedit.h"
#include "webview.h"

#include <qdrag.h>
#include <qevent.h>
#include <qpainter.h>
#include <qstyleoption.h>

#include <qdebug.h>

LocationBarSiteIcon::LocationBarSiteIcon(QWidget *parent)
    : QLabel(parent)
    , m_webView(0)
{
    resize(QSize(16, 16));
    webViewSiteIconChanged();
}

void LocationBarSiteIcon::setWebView(WebView *webView)
{
    m_webView = webView;
    connect(webView, SIGNAL(loadFinished(bool)),
            this, SLOT(webViewSiteIconChanged()));
    connect(webView, SIGNAL(iconChanged()),
            this, SLOT(webViewSiteIconChanged()));
}

void LocationBarSiteIcon::webViewSiteIconChanged()
{
    QUrl url;
    if (m_webView)
        url = m_webView->url();
    setPixmap(BrowserApplication::instance()->icon(url).pixmap(16, 16));
}

void LocationBarSiteIcon::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStartPos = event->pos();
    QLabel::mousePressEvent(event);
}

void LocationBarSiteIcon::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton
        && (event->pos() - m_dragStartPos).manhattanLength() > QApplication::startDragDistance()
        && m_webView) {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setText(m_webView->url().toString());
        QList<QUrl> urls;
        urls.append(m_webView->url());
        mimeData->setUrls(urls);
        drag->setMimeData(mimeData);
        drag->exec();
    }
}

LocationBar::LocationBar(QWidget *parent)
    : LineEdit(parent)
    , m_webView(0)
    , m_siteIcon(0)
{
    // Urls are always LeftToRight
    setLayoutDirection(Qt::LeftToRight);

    setUpdatesEnabled(false);
    // site icon on the left
    m_siteIcon = new LocationBarSiteIcon(this);
    addWidget(m_siteIcon, LeftSide);

    // clear button on the right
    ClearButton *m_clearButton = new ClearButton(this);
    connect(m_clearButton, SIGNAL(clicked()),
            this, SLOT(clear()));
    connect(this, SIGNAL(textChanged(const QString&)),
            m_clearButton, SLOT(textChanged(const QString&)));
    addWidget(m_clearButton, RightSide);
    m_clearButton->hide();
    updateTextMargins();
    setUpdatesEnabled(true);

    m_defaultBaseColor = palette().color(QPalette::Base);

    QPalette p = palette();
    p.setColor(QPalette::Base, QColor(255, 255, 255, 100));
    setPalette(p);
}

void LocationBar::setWebView(WebView *webView)
{
    Q_ASSERT(webView);
    m_webView = webView;
    m_siteIcon->setWebView(webView);
    connect(webView, SIGNAL(urlChanged(const QUrl &)),
            this, SLOT(webViewUrlChanged(const QUrl &)));
    connect(webView, SIGNAL(loadProgress(int)),
            this, SLOT(update()));
}

void LocationBar::webViewUrlChanged(const QUrl &url)
{
    setText(url.toString());
    setCursorPosition(0);
}

static QLinearGradient generateGradient(const QColor &top, const QColor &middle, int height)
{
    QLinearGradient gradient(0, 0, 0, height);
    gradient.setColorAt(0, top);
    gradient.setColorAt(0.15, middle.lighter(120));
    gradient.setColorAt(0.5, middle);
    gradient.setColorAt(0.85, middle.lighter(120));
    gradient.setColorAt(1, top);
    return gradient;
}

void LocationBar::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    QPalette p = palette();
    QColor backgroundColor = m_defaultBaseColor;
    if (m_webView && m_webView->url().scheme() == QLatin1String("https")
        && p.brush(QPalette::Text) != Qt::black) {
        QColor lightYellow(248, 248, 210);
        backgroundColor = lightYellow;
    }

    // paint the text background
    QStyleOptionFrameV2 panel;
    initStyleOption(&panel);
    QRect backgroundRect = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
    int left = textMargin(LineEdit::LeftSide);
    int right = textMargin(LineEdit::RightSide);
    backgroundRect.adjust(-left, 0, right, 0);
    painter.setBrush(backgroundColor);
    painter.setPen(backgroundColor);
    painter.drawRect(backgroundRect);

    // paint the progressbar
    if (m_webView && !hasFocus()) {
        int progress = m_webView->progress();
        QColor loadingColor = QColor(116, 192, 250);
        if (p.brush(QPalette::Text) != Qt::black)
            loadingColor = m_defaultBaseColor.value() < 128 ? m_defaultBaseColor.lighter(200) : m_defaultBaseColor.darker(200);

        painter.setBrush(generateGradient(m_defaultBaseColor, loadingColor, height()));
        painter.setPen(Qt::transparent);

        int mid = backgroundRect.width() * progress / 100;
        QRect progressRect = QRect(backgroundRect.x(), backgroundRect.y(), mid, backgroundRect.height());
        painter.drawRect(progressRect);
    }
    painter.end();

    LineEdit::paintEvent(event);
}

void LocationBar::focusOutEvent(QFocusEvent *event)
{
    if (text().isEmpty() && m_webView)
        webViewUrlChanged(m_webView->url());
    QLineEdit::focusOutEvent(event);
}
