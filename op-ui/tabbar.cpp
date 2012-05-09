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
**��Software��), to deal with the Software without restriction, including 
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

/****************************************************************************
**
** Copyright (C) 2007-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "tabbar.h"

#include <qaction.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qevent.h>
#include <qmenu.h>
#include <qstyle.h>
#include <qurl.h>

#include <qdebug.h>

TabShortcut::TabShortcut(int tab, const QKeySequence &key, QWidget *parent)
    : QShortcut(key, parent)
    , m_tab(tab)
{
}

int TabShortcut::tab()
{
    return m_tab;
}

TabBar::TabBar(QWidget *parent)
    : QTabBar(parent)
    , m_viewTabBarAction(0)
    , m_showTabBarWhenOneTab(true)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setAcceptDrops(true);
    setElideMode(Qt::ElideRight);
    setUsesScrollButtons(true);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(contextMenuRequested(const QPoint &)));

    QString alt = QLatin1String("Ctrl+%1");
    for (int i = 0; i < 10; ++i) {
        int key = i == 9 ? 0 : i + 1;
        TabShortcut *tabShortCut = new TabShortcut(i, alt.arg(key), this);
        connect(tabShortCut, SIGNAL(activated()), this, SLOT(selectTabAction()));
    }

    m_viewTabBarAction = new QAction(this);
    updateViewToolBarAction();
    connect(m_viewTabBarAction, SIGNAL(triggered()),
            this, SLOT(viewTabBar()));
#if QT_VERSION >= 0x040500
    setMovable(true);
#endif
}

bool TabBar::showTabBarWhenOneTab() const
{
    return m_showTabBarWhenOneTab;
}

void TabBar::setShowTabBarWhenOneTab(bool enabled)
{
    m_showTabBarWhenOneTab = enabled;
    updateVisibility();
}

QAction *TabBar::viewTabBarAction() const
{
    return m_viewTabBarAction;
}

#if QT_VERSION >= 0x040500
QTabBar::ButtonPosition TabBar::freeSide()
{
    QTabBar::ButtonPosition side = (QTabBar::ButtonPosition)style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, 0, this);
    side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
    return side;
}
#endif

void TabBar::updateViewToolBarAction()
{
    bool show = showTabBarWhenOneTab();
    if (count() > 1)
        show = true;
    m_viewTabBarAction->setText(!show ? tr("Show Tab Bar") : tr("Hide Tab Bar"));
}

void TabBar::viewTabBar()
{
    setShowTabBarWhenOneTab(!showTabBarWhenOneTab());
    updateViewToolBarAction();
}

void TabBar::selectTabAction()
{
    int index = qobject_cast<TabShortcut*>(sender())->tab();
    setCurrentIndex(index);
}

void TabBar::contextMenuRequested(const QPoint &position)
{
    QMenu menu;
    menu.addAction(tr("New &Tab"), this, SIGNAL(newTab()), QKeySequence::AddTab);
    int index = tabAt(position);
    if (-1 != index) {
        QAction *action = menu.addAction(tr("Duplicate Tab"),
                                         this, SLOT(cloneTab()));
        action->setData(index);

        menu.addSeparator();

        action = menu.addAction(tr("&Close Tab"),
                                this, SLOT(closeTab()), QKeySequence::Close);
        action->setData(index);

        action = menu.addAction(tr("Close &Other Tabs"),
                                this, SLOT(closeOtherTabs()));
        action->setData(index);

        menu.addSeparator();

        action = menu.addAction(tr("Reload Tab"),
                                this, SLOT(reloadTab()), QKeySequence::Refresh);
        action->setData(index);
    } else {
        menu.addSeparator();
    }
    menu.addAction(tr("Reload All Tabs"), this, SIGNAL(reloadAllTabs()));
    menu.exec(QCursor::pos());
}

void TabBar::cloneTab()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        int index = action->data().toInt();
        emit cloneTab(index);
    }
}

void TabBar::closeTab()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        int index = action->data().toInt();
        emit closeTab(index);
    }
}

void TabBar::closeOtherTabs()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        int index = action->data().toInt();
        emit closeOtherTabs(index);
    }
}

void TabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!childAt(event->pos())
        // Remove the line below when QTabWidget does not have a one pixel frame
        && event->pos().y() < (y() + height())) {
        emit newTab();
        return;
    }
    QTabBar::mouseDoubleClickEvent(event);
}

void TabBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MidButton) {
        int index = tabAt(event->pos());
        if (index != -1) {
            emit closeTab(index);
        } else {
            QUrl url(QApplication::clipboard()->text(QClipboard::Selection));
            if (!url.isEmpty() && url.isValid() && !url.scheme().isEmpty())
                emit loadUrl(url, TabWidget::NewTab);
        }
    }

    QTabBar::mouseReleaseEvent(event);
}

void TabBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStartPos = event->pos();
    QTabBar::mousePressEvent(event);
}

void TabBar::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
#if QT_VERSION >= 0x040500
        int diffX = event->pos().x() - m_dragStartPos.x();
        int diffY = event->pos().y() - m_dragStartPos.y();
#endif
        if ((event->pos() - m_dragStartPos).manhattanLength() > QApplication::startDragDistance()
#if QT_VERSION >= 0x040500
            && diffX < 3 && diffX > -3
            && diffY < -10
#endif
            ) {
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData;
            QList<QUrl> urls;
            int index = tabAt(event->pos());
            QUrl url = tabData(index).toUrl();
            urls.append(url);
            mimeData->setUrls(urls);
            mimeData->setText(tabText(index));
            mimeData->setData(QLatin1String("action"), "tab-reordering");
            drag->setMimeData(mimeData);
            drag->exec();
        }
    }
    QTabBar::mouseMoveEvent(event);
}

#if QT_VERSION < 0x040500
void TabBar::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    QStringList formats = mimeData->formats();
    if (formats.contains(QLatin1String("action"))
        && (mimeData->data(QLatin1String("action")) == "tab-reordering")) {
        event->acceptProposedAction();
    }
    QTabBar::dragEnterEvent(event);
}

void TabBar::dropEvent(QDropEvent *event)
{
    int fromIndex = tabAt(m_dragStartPos);
    int toIndex = tabAt(event->pos());
    if (fromIndex != toIndex) {
        emit tabMoveRequested(fromIndex, toIndex);
        event->acceptProposedAction();
    }
    QTabBar::dropEvent(event);
}
#endif

QSize TabBar::tabSizeHint(int index) const
{
    QSize sizeHint = QTabBar::tabSizeHint(index);
    QFontMetrics fm = fontMetrics();
    return sizeHint.boundedTo(QSize(fm.width(QLatin1Char('M')) * 18, sizeHint.height()));
}

void TabBar::reloadTab()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        int index = action->data().toInt();
        emit reloadTab(index);
    }
}

void TabBar::tabInserted(int position)
{
    Q_UNUSED(position);
    updateVisibility();
}

void TabBar::tabRemoved(int position)
{
    Q_UNUSED(position);
    updateVisibility();
}

void TabBar::updateVisibility()
{
    setVisible((count()) > 1 || m_showTabBarWhenOneTab);
    m_viewTabBarAction->setEnabled(count() == 1);
    updateViewToolBarAction();
}

