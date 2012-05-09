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

#include <QVariant>
#include <QBuffer>
#include <QDataStream>

#include "frameview.h"
#include "Message.h"
#include <QDebug>

QByteArray static getByteArray(const QVariant& var) {
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::ReadWrite);

    QDataStream out(&buffer);
    out << var;
    return byteArray;
}

FrameView::FrameView(QWidget* parent)
    : QX11EmbedContainer(parent)
    , m_frameName()
    , m_clientId(0)
    , m_clientWId(0)
{
    connect(this, SIGNAL(clientClosed()),
            this, SLOT(slotClientClosed()));
    connect(this, SIGNAL(clientIsEmbedded()),
            this, SLOT(slotClientIsEmbeded()));
}

FrameView::~FrameView()
{
    if(m_clientId != 0) {
        sendSysCall(MSG_WEBAPP_CLOSE, QByteArray::number(m_clientId));
        qDebug() << "FrameView close:" << m_clientId;
    }
}

void FrameView::setFrameName(QString name)
{
    m_frameName = name;
}

QString FrameView::getFrameName()
{
    return m_frameName;
}

void FrameView::setClientIds(int clientId, int clientWId)
{
    m_clientId = clientId;
    m_clientWId = clientWId;
    sendSysCall(MSG_UPDATA_CONTAINER, QByteArray::number(m_clientId));
}

void FrameView::handleEmbed(int clientId, int clientWId)
{
    int oldClientId = m_clientId;
    setClientIds(clientId, clientWId);
    // close the old one
    if(oldClientId != 0) {
        sendSysCall(MSG_WEBAPP_CLOSE, QByteArray::number(oldClientId));
        if (clientWinId() != 0) {
            discardClient();
        }
    } else {
        embedClient(clientWId);
    }
    // update container info after close old webapp
    
}

void FrameView::slotClientClosed()
{
    qDebug() << "iframe discard client";
    embedClient(m_clientWId);
}

void FrameView::slotClientIsEmbeded()
{

    if (m_clientWId != (int) clientWinId()) {
        discardClient();
    } else {
        QVariant var(size());
        sendWebAppMsg(MSG_WEBAPP_SHOW, getByteArray(var));
        // qDebug() << "frame is embedded " << m_clientWId;
    }
}

void FrameView::sendSysCall(int type, const QByteArray& content)
{
    Message msg;
    msg.setDstId(KERNEL_ID);
    msg.setMsgType(type);
    msg.setMsgValue(winId());
    msg.setData(content.constData(), content.size());
    msg.writeMessage();
}
    
void FrameView::sendWebAppMsg(int type, const QByteArray& content)
{
    if (m_clientId != 0) {
        Message msg;
        msg.setDstId(m_clientId);
        msg.setMsgType(MSG_WEBAPP_MSG);
        msg.setMsgValue(type);
        msg.setData(content.constData(), content.size());
        msg.writeMessage();
    }
}

