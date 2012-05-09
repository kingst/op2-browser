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

#include "OpDownloader.h"
#include "OpNetwork.h"
#include "OpNetworkReply.h"

#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDataStream>
#include <QBuffer>
#include <QVariant>


int OpDownloader::s_reqId = 0;

OpDownloader::OpDownloader(QNetworkReply* reply)
    : m_reply(reply)
    , m_reqId(s_reqId++)
    , m_sent(false)
    , m_finished(false)
    , m_bytes(0)
{
    init();
}

OpDownloader::OpDownloader(const QNetworkRequest& request)
    : m_reply(0)
    , m_reqId(s_reqId++)
    , m_sent(false)
    , m_finished(false)
    , m_bytes(0)
{
    if (!request.url().isEmpty())
        m_reply = OPNET::OpNetwork::instance()->networkAccessManager()->get(request);
    init();
}

OpDownloader::~OpDownloader()
{
}

void OpDownloader::init()
{
    m_bytes = 0;
    if (m_reply == 0)
        finished();
    m_reply->setParent(this);
    connect(m_reply, SIGNAL(readyRead()), this, SLOT(downloadReadyRead()));
    connect(m_reply, SIGNAL(metaDataChanged()),
            this, SLOT(metaDataChanged()));
    connect(m_reply, SIGNAL(finished()),
            this, SLOT(finished()));
    
    if (m_reply->error() != QNetworkReply::NoError) {
        finished();
    } else {
        OPNET::OpNetworkReply* opReply = qobject_cast<OPNET::OpNetworkReply*> (m_reply);
        if (opReply != 0 && opReply->isFinished()) {
            start();
            downloadReadyRead();
            finished();
        }
    }
}

QString OpDownloader::getFileName()
{
    // Move this function into QNetworkReply to also get file name sent from the server
    QString path;
    if (m_reply->hasRawHeader("Content-Disposition")) {
        QString value = QLatin1String(m_reply->rawHeader("Content-Disposition"));
        int pos = value.indexOf(QLatin1String("filename="));
        if (pos != -1) {
            QString name = value.mid(pos + 9);
            if (name.startsWith(QLatin1Char('"')) && name.endsWith(QLatin1Char('"')))
                name = name.mid(1, name.size() - 2);
            path = name;
        }
    }
    if (path.isEmpty())
        path = m_reply->url().path();

    QFileInfo info(path);
    QString baseName = info.completeBaseName();
    QString endName = info.suffix();

    if (baseName.isEmpty()) {
        baseName = QLatin1String("unknown");
    }

    if (!endName.isEmpty()) {
        endName = QLatin1Char('.') + endName;
    } else {
        
    }

    QString name = baseName + endName;
    return name;
}

void OpDownloader::downloadReadyRead()
{
    QByteArray data = m_reply->readAll();

    if (data.size() > 0) {
        if (!m_sent) {
            // if we haven't sent the request
            start();
        }
        m_bytes += data.size();
        OPNET::OpNetwork::instance()->sendDownloadRequest(m_reqId, data);
    }
}

void OpDownloader::start()
{
    qint64 size = m_reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadWrite);
    QDataStream out(&buffer);
    QVariant var;

    m_filename = getFileName();
    
    var = QVariant(m_filename);
    out << var;
    var = QVariant(size);
    out << var;

    OPNET::OpNetwork::instance()->sendDownloadRequest(m_reqId, bytes);
    m_sent = true;
}

void OpDownloader::metaDataChanged()
{
}

void OpDownloader::finished()
{
    if (m_sent && !m_finished) {
        OPNET::OpNetwork::instance()->sendDownloadRequest(m_reqId, QByteArray());
    }
    m_finished = true;
    deleteLater();   
}

