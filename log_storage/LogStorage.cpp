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
#include "LogStorage.h"
#include "../kernel/Message.h"

#include <iostream>
#include <signal.h>
#include <string>
#include <sqlite3.h>

#include <stdio.h>
#include <errno.h>
#include <assert.h>

using namespace std;

#define LOG_STORE_FILE "auditlog.db"

LogStorage::LogStorage() {
    int rc;
    char *zErrMsg = NULL;

    m_stmt = NULL;
    m_sessionStart = -1;

    rc = sqlite3_open(LOG_STORE_FILE, &m_db);
    if( rc ) {
        cerr << "can't open database " << LOG_STORE_FILE << endl;
        m_db = NULL;
        return;
    }

    rc = sqlite3_exec(m_db, "create table if not exists auditlog (srcId integer, dstId integer, msgId integer, msgType integer, msgValue integer, dataLen integer, msgData text, ts_sec integer, ts_usec integer)", NULL, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
        cerr << "could not create table: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        sqlite3_close(m_db);
        m_db = NULL;
    }

    rc = sqlite3_exec(m_db, "create table if not exists sessions (sessionStart integer)", NULL, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
        cerr << "could not create table: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        sqlite3_close(m_db);
        m_db = NULL;
    }

    rc = sqlite3_exec(m_db, "PRAGMA synchronous=OFF", NULL, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
        cerr << "could not turn off synchronous writes: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        sqlite3_close(m_db);
        m_db = NULL;
    }
}

LogStorage::~LogStorage() {
    if(m_stmt != NULL) {
        sqlite3_finalize(m_stmt);
        m_stmt = NULL;
    }

    if(m_db != NULL) {
        sqlite3_close(m_db);
        m_db = NULL;
    }
}


void LogStorage::insertSessionStart(i64 sessionStart) {
    int rc;
    char *zErrMsg = NULL;
    string qStr;
    char idStr[32];

    sprintf(idStr, "%ld", sessionStart);
    qStr = string("insert into sessions values (") + string(idStr) + string(")");

    rc = sqlite3_exec(m_db, qStr.c_str(), NULL, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
        cerr << "could not insert session id: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        assert(false);
    }
}

void LogStorage::putMessage(Message *msg) {
    int ret;

    if(m_stmt != NULL) {
        sqlite3_finalize(m_stmt);
        m_stmt = NULL;
    }

    ret = sqlite3_prepare(m_db, "insert into auditlog values (?, ?, ?, ?, ?, ?, ?, ?, ?)", -1, 
                          &m_stmt,0);
    if(ret != SQLITE_OK) {
        cerr << "logstoreage: coudl not put message ret = " << ret << endl;
        exit(1);
    }

    ret = sqlite3_bind_int(m_stmt, 1, msg->getSrcId()); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_int(m_stmt, 2, msg->getDstId()); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_int64(m_stmt, 3, msg->getMsgId()); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_int(m_stmt, 4, msg->getMsgType()); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_int(m_stmt, 5, msg->getMsgValue()); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_int(m_stmt, 6, msg->getDataLen()); assert(ret == SQLITE_OK);

    if(msg->getDataLen() > 0) {
        ret = sqlite3_bind_text(m_stmt, 7, (const char *) msg->getMsgData(), 
                                msg->getDataLen(), SQLITE_TRANSIENT);
        assert(ret == SQLITE_OK);
    } else {
        ret = sqlite3_bind_null(m_stmt, 7);
        assert(ret == SQLITE_OK);
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    ret = sqlite3_bind_int64(m_stmt, 8, tv.tv_sec); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_int64(m_stmt, 9, tv.tv_usec); assert(ret == SQLITE_OK);

    if(sqlite3_step(m_stmt) != SQLITE_DONE) {
        cerr << "logstorage: could not execute putMesage query" << endl;
        sqlite3_finalize(m_stmt);
        m_stmt = NULL;
    }

    sqlite3_finalize(m_stmt);
    if(m_sessionStart < 0) {
        m_sessionStart = sqlite3_last_insert_rowid(m_db);
        insertSessionStart(m_sessionStart);
    }
    m_stmt = NULL;
}

void LogStorage::initGetObjectAuditRecords(i32 objectId) {
    int ret;
    char idStr[32], sStr[32];
    string queryString;    

    if(m_stmt != NULL) {
        sqlite3_finalize(m_stmt);
        m_stmt = NULL;
    }

    assert(m_sessionStart >= 0);

    sprintf(idStr, "%d", objectId);
    sprintf(sStr, "%ld", m_sessionStart);
    queryString = string("select * from auditlog where (srcId = ") +
        string(idStr) + string(" or dstId = ") + string(idStr) +
        string(") and ROWID >= ") + string(sStr) + string(" order by msgId asc");

    cerr << "initGetObjectAuditRecords query " << queryString << endl;
    ret = sqlite3_prepare(m_db, queryString.c_str(), -1, &m_stmt, 0);
    if(ret != SQLITE_OK) {
        cerr << "logstoreage: coudl not init message ret = " << ret << endl;
        exit(1);
    }    
}

bool LogStorage::getNextMessage(Message *msg) {
    int ret;

    if(m_stmt == NULL) {
        return false;
    }

    ret = sqlite3_step(m_stmt);
    if(ret == SQLITE_DONE) {
        sqlite3_finalize(m_stmt);
        m_stmt = NULL;
        return false;
    } else if(ret == SQLITE_ROW) {
        msg->setSrcId(sqlite3_column_int(m_stmt, 0));
        msg->setDstId(sqlite3_column_int(m_stmt, 1));
        msg->setMsgId(sqlite3_column_int64(m_stmt, 2));
        msg->setMsgType(sqlite3_column_int(m_stmt, 3));
        msg->setMsgValue(sqlite3_column_int(m_stmt, 4));        
        msg->setData(sqlite3_column_text(m_stmt, 6), sqlite3_column_bytes(m_stmt, 6));
    } else {
        cerr << "error with getNextMessage" << endl;
        exit(1);
    }

    return true;
}
