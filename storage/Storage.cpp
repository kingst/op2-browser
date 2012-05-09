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
#include "Storage.h"
#include "Message.h"
#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QDataStream>
#include <QBuffer>
#include <QVariant>


Storage* Storage::_instance = NULL;
QString Storage::_dbName = "storage.db";


Storage* Storage::instance()
{
    if (_instance == NULL) {
        _instance = new Storage(_dbName);
    }
    return _instance;
}

void Storage::release()
{
    if (_instance != NULL) {
        delete _instance;
        _instance = NULL;
    }
}

Storage::Storage(const QString& dbName)
    : m_dbName(dbName)
    , m_db(NULL)
{
    if (!init()) {
        QApplication::exit(1);
    }
}

Storage::~Storage()
{
    sqlite3_close(m_db);
}

bool Storage::init()
{
    int rc;

    rc = sqlite3_open(m_dbName.toAscii().data(), &m_db);
    if (rc != SQLITE_OK) {
        qDebug() << "Cannot open database: " << sqlite3_errmsg(m_db);
        m_db = NULL;
        return false;
    }

    // we need to create table for configuration etc.
    // but currently, we don't support this feature
    return true;
}

inline static int sqliteCallBack(void* notUsed, int argc, char** argv, char** azColName) {
    Q_UNUSED(notUsed);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    Q_UNUSED(azColName);
    return 0;
}

bool Storage::query(const QString& stmt)
{
    char *zErrMsg = 0;
    int rc;

    if (m_db == NULL)
        return false;

    rc = sqlite3_exec(m_db, stmt.toLatin1().constData(), sqliteCallBack, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        qDebug() << "SQL error: " << zErrMsg;
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

void Storage::writeFile(int webappId, int fileId, const QByteArray& data)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadWrite);
    QDataStream out(&buffer);
    QVariant var;
        
    QPair<int, int> id(webappId, fileId);
    if (m_writeQueue.find(id) == m_writeQueue.end()) {
        QDataStream in(data);
        QVariant var;
        in >> var;
        QString fileName = var.toString();
        in >> var;
        qint64 fileSize = var.toLongLong();

        QFileInfo info(fileName);
        QString baseName = info.completeBaseName();
        QString endName = info.suffix();
        if (baseName.isEmpty()) {
            baseName = QLatin1String("unknown");
        }
        
        if (!endName.isEmpty())
            endName = QLatin1Char('.') + endName;

        QString actualName = baseName + endName;
        if (QFile::exists(actualName)) {
            int i = 1;
            do {
                actualName = baseName + QLatin1Char('-') + QString::number(i++) + endName;
            } while (QFile::exists(actualName));
        }

        // create new file
        m_writeQueue[id] = new QFile(actualName);
        m_writeQueue[id]->open(QIODevice::WriteOnly);
        
        var = QVariant(actualName);
        out << var;
        var = QVariant(fileSize);
        out << var;
    } else {
        if (data.size() > 0) {
            if (m_writeQueue[id] != NULL && m_writeQueue[id]->isOpen()) {
                m_writeQueue[id]->write(data);
                var = QVariant(data.size());
                out << var;
            } else {
                var = QVariant(0);
                out << var;
            }
        } else {
            if (m_writeQueue[id] != NULL) {
                m_writeQueue[id]->close();
            }
            var = QVariant(0);
            out << var;
        }
    }

    // tell UI
    Message msg(STORAGE_ID, UI_ID, 0, MSG_DOWNLOAD_INFO,
              webappId * 1024 + fileId, bytes.constData(), bytes.size());
    msg.writeMessage();

}

QByteArray Storage::readFile(const QByteArray& filename)
{
    Q_UNUSED(filename);
    return QByteArray();
}
