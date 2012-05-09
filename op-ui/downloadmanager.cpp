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
 * Copyright 2008 Jason A. Donenfeld <Jason@zx2c4.com>
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
** Copyright (C) 2008-2008 Trolltech ASA. All rights reserved.
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

#include "downloadmanager.h"

#include "autosaver.h"
#include "browserapplication.h"

#include <math.h>

//#include <qdesktopservices.h>
#include <qfiledialog.h>
#include <qfileiconprovider.h>
#include <qheaderview.h>
#include <qmetaobject.h>
#include <qsettings.h>

#include <QDataStream>
#include <QBuffer>
#include <QVariant>

#include <qdebug.h>

/*!
    DownloadItem is a widget that is displayed in the download manager list.
    It moves the data from the QNetworkReply into the QFile as well
    as update the information/progressbar and report errors.
 */

DownloadItem::DownloadItem(int jid, const QString& filename,
                           qint64 bytesTotal, QWidget* parent) 
    : QWidget(parent)
    , m_bytesReceived(0)
    , m_fileName(filename)
    , m_jobId(jid)
    , m_finishedDownloading(false)
{
    setupUi(this);

    fileNameLabel->setText(filename);
    
    QPalette p = downloadInfoLabel->palette();
    p.setColor(QPalette::Text, Qt::darkGray);
    downloadInfoLabel->setPalette(p);
    progressBar->setMaximum(bytesTotal);
    tryAgainButton->hide();

    // reset info
    downloadInfoLabel->clear();
    progressBar->setValue(0);

    m_downloadTime.start();
}

/*
void DownloadItem::stop()
{
    setUpdatesEnabled(false);
    stopButton->setEnabled(false);
    stopButton->hide();
    tryAgainButton->setEnabled(true);
    tryAgainButton->show();
    setUpdatesEnabled(true);
    m_reply->abort();
}

void DownloadItem::open()
{
    QFileInfo info(m_output);
    QUrl url = QUrl::fromLocalFile(info.absolutePath());
    QDesktopServices::openUrl(url);
}

void DownloadItem::tryAgain()
{
    if (!tryAgainButton->isEnabled())
        return;

    tryAgainButton->setEnabled(false);
    tryAgainButton->setVisible(false);
    stopButton->setEnabled(true);
    stopButton->setVisible(true);
    progressBar->setVisible(true);

    QNetworkReply *r = BrowserApplication::networkAccessManager()->get(QNetworkRequest(m_url));
    if (m_reply)
        m_reply->deleteLater();
    if (m_output.exists())
        m_output.remove();
    m_reply = r;
    init();
    emit statusChanged();
}

void DownloadItem::error(QNetworkReply::NetworkError)
{
    qDebug() << "DownloadItem::error" << m_reply->errorString() << m_url;
    downloadInfoLabel->setText(tr("Network Error: %1").arg(m_reply->errorString()));
    tryAgainButton->setEnabled(true);
    tryAgainButton->setVisible(true);
}*/

void DownloadItem::downloadProgress(qint64 bytesReceived)
{
    qint64 bytesTotal = progressBar->maximum();
    m_bytesReceived += bytesReceived;
    if (bytesTotal == -1) {
        progressBar->setValue(0);
        progressBar->setMaximum(0);
    } else {
        progressBar->setValue(m_bytesReceived);
        progressBar->setMaximum(bytesTotal);
    }
    updateInfoLabel();
}

void DownloadItem::updateInfoLabel()
{
    qint64 bytesTotal = progressBar->maximum();
    bool running = !downloadedSuccessfully();

    // update info label
    double speed = m_bytesReceived * 1000.0 / m_downloadTime.elapsed();
    double timeRemaining = ((double)(bytesTotal - m_bytesReceived)) / speed;
    QString timeRemainingString = tr("seconds");


    // When downloading the eta should never be 0
    if (timeRemaining == 0)
        timeRemaining = 1;

    QString info;
    if (running) {
        QString remaining;

        if (bytesTotal != 0) {
            if (timeRemaining > 60) {
                timeRemaining = timeRemaining / 60;
                timeRemaining = floor(timeRemaining);
                remaining = tr("- %n minutes remaining", "", timeRemaining);
            }
            else {
                timeRemaining = floor(timeRemaining);
                remaining = tr("- %n seconds remaining", "", timeRemaining);
            }
        }
        info = QString(tr("%1 of %2 (%3/sec) %4"))
            .arg(dataString(m_bytesReceived))
            .arg(bytesTotal == 0 ? tr("?") : dataString(bytesTotal))
            .arg(dataString((int)speed))
            .arg(remaining);
    } else {
        if (m_bytesReceived == bytesTotal)
            info = dataString(bytesTotal);
        else
            info = tr("%1 of %2 - Stopped")
                .arg(dataString(m_bytesReceived))
                .arg(dataString(bytesTotal));
    }
    downloadInfoLabel->setText(info);
    emit statusChanged();
}

QString DownloadItem::dataString(int size) const
{
    QString unit;
    double newSize;

    if (size < 1024) {
        newSize = size;
        unit = tr("bytes");
    } else if (size < 1024 * 1024) {
        newSize = (double)size / (double)1024;
        unit = tr("kB");
    } else {
        newSize = (double)size / (double)(1024 * 1024);
        unit = tr("MB");
    }
    return QString(QLatin1String("%1 %2")).arg(newSize, 0, 'f', 1).arg(unit);
}

int DownloadItem::jobId() const
{
    return m_jobId;
}

bool DownloadItem::downloading() const
{
    return (progressBar->isVisible());
}

bool DownloadItem::downloadedSuccessfully() const
{
    return (stopButton->isHidden() && tryAgainButton->isHidden());
}

void DownloadItem::finished()
{
    m_finishedDownloading = true;
    progressBar->hide();
    stopButton->setEnabled(false);
    stopButton->hide();
    updateInfoLabel();
    emit statusChanged();
}

QString DownloadItem::fileName() const
{
    return m_fileName;
}

/*!
    DownloadManager is a Dialog that contains a list of DownloadItems

    It is a basic download manager.  It only downloads the file, doesn't do BitTorrent,
    extract zipped files or anything fancy.
  */
DownloadManager::DownloadManager(QWidget *parent)
    : QDialog(parent)
    , m_autoSaver(new AutoSaver(this))
    , m_iconProvider(0)
    , m_removePolicy(Never)
{
    setupUi(this);
    downloadsView->setShowGrid(false);
    downloadsView->verticalHeader()->hide();
    downloadsView->horizontalHeader()->hide();
    downloadsView->setAlternatingRowColors(true);
    downloadsView->horizontalHeader()->setStretchLastSection(true);
    m_model = new DownloadModel(this);
    downloadsView->setModel(m_model);
    connect(cleanupButton, SIGNAL(clicked()), this, SLOT(cleanup()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
    // load();
}

DownloadManager::~DownloadManager()
{
    // m_autoSaver->changeOccurred();
    // m_autoSaver->saveIfNeccessary();
    if (m_iconProvider)
        delete m_iconProvider;
}

int DownloadManager::activeDownloads() const
{
    int count = 0;
    for (int i = 0; i < m_downloads.count(); ++i) {
        if (m_downloads.at(i)->stopButton->isEnabled())
            ++count;
    }
    return count;
}


void DownloadManager::handleDownloadInfo(int jobId, const QByteArray& data)
{
    DownloadItem* item = NULL;
    for (int i = 0; i < m_downloads.count(); ++i) {
        if (m_downloads.at(i)->jobId() == jobId) {
            item = m_downloads.at(i);
            break;
        }
    }

    QDataStream in(data);
    QVariant var;
    
    if (item == NULL) {
        in >> var;
        QString filename = var.toString();
        in >> var;
        qint64 bytesTotal = var.toLongLong();
        addItem(new DownloadItem(jobId, filename, bytesTotal, this));

        if (!isVisible())
            show();

        activateWindow();
        raise();
        
    } else {
        in >> var;
        int num = var.toInt();
        if (num > 0)
            item->downloadProgress(num);
        else
            item->finished();
    }
}

void DownloadManager::addItem(DownloadItem *item)
{
    connect(item, SIGNAL(statusChanged()), this, SLOT(updateRow()));
    int row = m_downloads.count();
    m_model->beginInsertRows(QModelIndex(), row, row);
    m_downloads.append(item);
    m_model->endInsertRows();
    updateItemCount();
    if (row == 0)
        show();
    downloadsView->setIndexWidget(m_model->index(row, 0), item);
    QIcon icon = style()->standardIcon(QStyle::SP_FileIcon);
    item->fileIcon->setPixmap(icon.pixmap(48, 48));
    downloadsView->setRowHeight(row, item->sizeHint().height());
    updateRow(item); //incase download finishes before the constructor returns
}

void DownloadManager::updateRow()
{
    if (DownloadItem *item = qobject_cast<DownloadItem*>(sender()))
        updateRow(item);
}


void DownloadManager::updateRow(DownloadItem *item)
{
    int row = m_downloads.indexOf(item);
    if (-1 == row)
        return;        
    if (!m_iconProvider)
        m_iconProvider = new QFileIconProvider();
    QIcon icon = m_iconProvider->icon(item->fileName());
    if (icon.isNull())
        icon = style()->standardIcon(QStyle::SP_FileIcon);
    item->fileIcon->setPixmap(icon.pixmap(48, 48));

    int oldHeight = downloadsView->rowHeight(row);
    downloadsView->setRowHeight(row, qMax(oldHeight, item->minimumSizeHint().height()));

    /*    bool remove = false;
    QWebSettings *globalSettings = QWebSettings::globalSettings();
    if (!item->downloading()
        && globalSettings->testAttribute(QWebSettings::PrivateBrowsingEnabled))
        remove = true;

    if (item->downloadedSuccessfully()
        && removePolicy() == DownloadManager::SuccessFullDownload) {
        remove = true;
    }
    if (remove)
    m_model->removeRow(row);*/

    cleanupButton->setEnabled(m_downloads.count() - activeDownloads() > 0);
} 

DownloadManager::RemovePolicy DownloadManager::removePolicy() const
{
    return m_removePolicy;
}

void DownloadManager::setRemovePolicy(RemovePolicy policy)
{
    if (policy == m_removePolicy)
        return;
    m_removePolicy = policy;
    m_autoSaver->changeOccurred();
}
/*
void DownloadManager::save() const
{
    QSettings settings;
    settings.beginGroup(QLatin1String("downloadmanager"));
    QMetaEnum removePolicyEnum = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("RemovePolicy"));
    settings.setValue(QLatin1String("removeDownloadsPolicy"), QLatin1String(removePolicyEnum.valueToKey(m_removePolicy)));
    settings.setValue(QLatin1String("size"), size());
    if (m_removePolicy == Exit)
        return;

    for (int i = 0; i < m_downloads.count(); ++i) {
        QString key = QString(QLatin1String("download_%1_")).arg(i);
        settings.setValue(key + QLatin1String("url"), m_downloads[i]->m_url);
        settings.setValue(key + QLatin1String("location"), QFileInfo(m_downloads[i]->m_output).filePath());
        settings.setValue(key + QLatin1String("done"), m_downloads[i]->downloadedSuccessfully());
    }
    int i = m_downloads.count();
    QString key = QString(QLatin1String("download_%1_")).arg(i);
    while (settings.contains(key + QLatin1String("url"))) {
        settings.remove(key + QLatin1String("url"));
        settings.remove(key + QLatin1String("location"));
        settings.remove(key + QLatin1String("done"));
        key = QString(QLatin1String("download_%1_")).arg(++i);
    }
    }*/
 /*
void DownloadManager::load()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("downloadmanager"));
    QSize size = settings.value(QLatin1String("size")).toSize();
    if (size.isValid())
        resize(size);
    QByteArray value = settings.value(QLatin1String("removeDownloadsPolicy"), QLatin1String("Never")).toByteArray();
    QMetaEnum removePolicyEnum = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("RemovePolicy"));
    m_removePolicy = removePolicyEnum.keyToValue(value) == -1 ?
                        Never :
                        static_cast<RemovePolicy>(removePolicyEnum.keyToValue(value));

    int i = 0;
    QString key = QString(QLatin1String("download_%1_")).arg(i);
    while (settings.contains(key + QLatin1String("url"))) {
        QUrl url = settings.value(key + QLatin1String("url")).toUrl();
        QString fileName = settings.value(key + QLatin1String("location")).toString();
        bool done = settings.value(key + QLatin1String("done"), true).toBool();
        if (!url.isEmpty() && !fileName.isEmpty()) {
            DownloadItem *item = new DownloadItem(0, this);
            item->m_output.setFileName(fileName);
            item->fileNameLabel->setText(QFileInfo(item->m_output.fileName()).fileName());
            item->m_url = url;
            addItem(item);
            item->stopButton->setVisible(false);
            item->stopButton->setEnabled(false);
            item->tryAgainButton->setVisible(!done);
            item->tryAgainButton->setEnabled(!done);
            item->progressBar->setVisible(false);
        }
        key = QString(QLatin1String("download_%1_")).arg(++i);
    }
    cleanupButton->setEnabled(m_downloads.count() - activeDownloads() > 0);
    } */

void DownloadManager::cleanup()
{
    if (m_downloads.isEmpty())
        return;
    m_model->removeRows(0, m_downloads.count());
    updateItemCount();
    if (m_downloads.isEmpty() && m_iconProvider) {
        delete m_iconProvider;
        m_iconProvider = 0;
    }
    m_autoSaver->changeOccurred();
}

void DownloadManager::updateItemCount()
{
    int count = m_downloads.count();
    itemCount->setText(tr("%n Download(s)", "", count));
}

DownloadModel::DownloadModel(DownloadManager *downloadManager, QObject *parent)
    : QAbstractListModel(parent)
    , m_downloadManager(downloadManager)
{
}

QVariant DownloadModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount(index.parent()))
        return QVariant();
    if (role == Qt::ToolTipRole)
        if (!m_downloadManager->m_downloads.at(index.row())->downloadedSuccessfully())
            return m_downloadManager->m_downloads.at(index.row())->downloadInfoLabel->text();
    return QVariant();
}

int DownloadModel::rowCount(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : m_downloadManager->m_downloads.count();
}

bool DownloadModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    int lastRow = row + count - 1;
    for (int i = lastRow; i >= row; --i) {
        if (m_downloadManager->m_downloads.at(i)->downloadedSuccessfully()
            || m_downloadManager->m_downloads.at(i)->tryAgainButton->isEnabled()) {
            beginRemoveRows(parent, i, i);
            m_downloadManager->m_downloads.takeAt(i)->deleteLater();
            endRemoveRows();
        }
    }
    m_downloadManager->m_autoSaver->changeOccurred();
    return true;
}

