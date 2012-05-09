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
#include "Network.h"
#include "Manager.h"
#include "Message.h"
#include "mime_sniffer.h"

#include <QDataStream>
#include <QBuffer>
#include <QVariant>
#include <QMap>
#include <QFile>
#include <QDebug>


NetworkRequest::NetworkRequest(Message* msg)
    : QObject(0)
    , m_metaDataSent(false)
    , m_postData()
{
    //Q_UNUSED(idToChar);

    QByteArray data((char*) msg->getMsgData(), msg->getDataLen());
    QDataStream in(data);
    QVariant var;

    in >> var;
    m_method = (QNetworkAccessManager::Operation) var.toInt();

    in >> var;
    QUrl url = var.toUrl();
    m_request = QNetworkRequest(url);
    
    in >> var;
    QMap<QString, QVariant> headers = var.toMap();
    QMap<QString, QVariant>::const_iterator iter = headers.constBegin();
    while (iter != headers.constEnd()) {
        m_request.setRawHeader(iter.key().toLatin1(), iter.value().toByteArray());
        iter ++;
    }

    in >> var;
    m_postData = var.toByteArray();

    m_urlId = msg->getMsgValue();
    m_srcId = msg->getSrcId();

    start();
}

void NetworkRequest::start()
{
    m_metaDataSent = false;
    QNetworkAccessManager* manager = Manager::instance()->networkAccessManager();
    switch (m_method) {
    case QNetworkAccessManager::GetOperation:
        m_reply = manager->get(m_request);
        //qDebug() << "Get:" << m_request.url().toString();
        break;
    case QNetworkAccessManager::PostOperation:
        processPostData();
        m_reply = manager->post(m_request, m_postData);
        //qDebug() << "Post:" << m_request.url().toString() << m_postData;
        break;
    default:
        break;
        qDebug() << "Unhandled operation.";
    }

    m_reply->setParent(this);

    connect(m_reply, SIGNAL(finished()),
            this, SLOT(finish()));

    QString scheme = m_request.url().scheme();
    if (scheme == QLatin1String("http") || scheme == QLatin1String("https"))
        connect(m_reply, SIGNAL(metaDataChanged()),
                this, SLOT(sendMetaDataIfNeeded()));

    connect(m_reply, SIGNAL(readyRead()),
            this, SLOT(forwardData()));
}

void NetworkRequest::processPostData()
{
    if (m_method != QNetworkAccessManager::PostOperation
        || m_postData.size() <= 0)
        return;
    QByteArray fnBegin = QByteArray("*OPFNBEGIN#");
    QByteArray fnEnd = QByteArray("*OPFNEND#");
    int beginIdx = m_postData.indexOf(fnBegin);
    int endIdx = m_postData.indexOf(fnEnd);
    while (endIdx - beginIdx > 0) {
        QString fileName = m_postData.mid(beginIdx+fnBegin.size(), endIdx-beginIdx-fnBegin.size());
        // in the future, we read file through storage subsystem
        QFile file(fileName);
        file.open(QFile::ReadOnly);
        QByteArray content;
        if (file.isOpen()) {
            // for large file, we should not read all at one time, in order not to block the subsystem...
            content = file.readAll();
            if (content.indexOf(fnBegin) >= 0 || content.indexOf(fnEnd) >= 0)
                content = QByteArray();
        }
        file.close();
        m_postData.replace(beginIdx, endIdx+fnEnd.size()-beginIdx, content);
        beginIdx = m_postData.indexOf(fnBegin);
        endIdx = m_postData.indexOf(fnEnd);
    }
}

void NetworkRequest::finish()
{
    sendMetaDataIfNeeded();

    QByteArray end;
    Message msg(NETWORK_ID, m_srcId, 0, MSG_RETURN_URL, m_urlId,
                end.constData(), end.size());
    msg.writeMessage();

    std::string mimeType;
    std::string headerMimeType = m_reply->header(QNetworkRequest::ContentTypeHeader).toString().toStdString();
    SniffMimeType(m_mimeData.constData(), m_mimeData.size(), 
                  m_request.url(), headerMimeType,
                  &mimeType); 
   
    if(headerMimeType != mimeType) {
        // Print WARNING
        //qDebug() << "MIME TYPE MISMATCH:" << m_request.url().toString();    
        //qDebug() << "Header MIME type: " << headerMimeType.c_str() << " sniffed mimeType: " << mimeType.c_str();
    }
    Manager::instance()->removeRequest(this);
    deleteLater();
}

void NetworkRequest::sendMetaDataIfNeeded()
{
    if (m_metaDataSent)
        return;
    m_metaDataSent = true;


    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadWrite);
    QDataStream out(&buffer);
    QVariant var;

    QList<QVariant> attributes;
    for (int code = (int) QNetworkRequest::HttpStatusCodeAttribute;
         code <= (int) QNetworkRequest::ConnectionEncryptedAttribute;
         code++) {
        attributes.push_back(m_reply->attribute((QNetworkRequest::Attribute)code));
    }
    var = QVariant(attributes);
    out << var;

    QMap<QString, QVariant> headers;
    foreach (QByteArray headerName, m_reply->rawHeaderList()) {
        headers[headerName] = QVariant(m_reply->rawHeader(headerName));
    }
    var = QVariant(headers);
    out << var;

    var = QVariant((int) m_reply->error());
    out << var;

    var = QVariant(m_reply->errorString());
    out << var;
    
    
    Message msg(NETWORK_ID, m_srcId, 0, MSG_RETURN_URL_METADATA, m_urlId,
                bytes.constData(), bytes.size());
    msg.writeMessage();
}

void NetworkRequest::forwardData()
{
    sendMetaDataIfNeeded();

    QByteArray data = m_reply->read(m_reply->bytesAvailable());
    if (!data.isEmpty()) {
        if(m_mimeData.size() < 512)
            m_mimeData += data;
        
        Message msg(NETWORK_ID, m_srcId, 0, MSG_RETURN_URL, m_urlId,
                    data.constData(), data.size());
        msg.writeMessage();  
    }
	
}

void NetworkRequest::abort()
{
    Manager::instance()->removeRequest(this);
    m_reply->abort();
    deleteLater();
}
