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
#include "StorageManager.h"
#include "Message.h"
#include "Storage.h"
#include <QDataStream>
#include <QBuffer>
#include <QVariant>
#include <QDebug>

StorageManager* StorageManager::_instance = NULL;

StorageManager* StorageManager::instance()
{
    if (_instance == NULL)
        _instance = new StorageManager();
    return _instance;
}

StorageManager::StorageManager()
    : QObject(0)
{
    m_stdin = new QSocketNotifier(Message::getReadFd(), QSocketNotifier::Read, NULL);
    connect(m_stdin, SIGNAL(activated(int)),
            this, SLOT(activated(int)));
    m_store = Storage::instance();
}

StorageManager::~StorageManager()
{
    Storage::release();
    m_stdin->deleteLater();
}

void StorageManager::activated(int fd)
{
    if (fd == Message::getReadFd()) {
        m_stdin->setEnabled(false);
        Message msg;
        while (msg.hasDataAvailable()) {
            msg.readMessage();
            dispatchMsg(&msg);
        }
        m_stdin->setEnabled(true);
    }
}

void StorageManager::dispatchMsg(Message* msg)
{
    QByteArray data((char*) msg->getMsgData(), msg->getDataLen());
    
    if (msg->getMsgType() == MSG_WRITE_FILE) {
        m_store->writeFile(msg->getSrcId(), msg->getMsgValue(), data);
	
    } else if (msg->getMsgType() == MSG_READ_FILE) {
        QByteArray filename = data;
        QByteArray bytes = m_store->readFile(filename);
        
        Message reply(STORAGE_ID, msg->getSrcId(), 0, MSG_READ_FILE_RETURN,
                      msg->getMsgValue(), bytes.constData(), bytes.size());
        reply.writeMessage();
        
    } else {
        qDebug() << "Unhandled Message Type in StorageManager";
    }
    
    return;
}

