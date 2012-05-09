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
#include "DumpStorage.h"
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

DumpStorage::DumpStorage() {
    int rc;
    char *zErrMsg = NULL;

    m_stmt = NULL;
    m_sessionStart = -1;
    m_sessionEnd = -1;

    rc = sqlite3_open(LOG_STORE_FILE, &m_db);
    if( rc ) {
        cerr << "can't open database " << LOG_STORE_FILE << endl;
        m_db = NULL;
        return;
    }
}

DumpStorage::~DumpStorage() {
    if(m_stmt != NULL) {
        sqlite3_finalize(m_stmt);
        m_stmt = NULL;
    }

    if(m_db != NULL) {
        sqlite3_close(m_db);
        m_db = NULL;
    }
}

void DumpStorage::initGetSessions() {
    int ret;
    
    if(m_stmt != NULL) {
        sqlite3_finalize(m_stmt);
        m_stmt = NULL;
    }

    ret = sqlite3_prepare(m_db, "select sessionStart, timestamp from sessions, auditlog where sessions.sessionStart = auditlog.ROWID", -1, &m_stmt, 0);
    assert(ret == SQLITE_OK);
}

bool DumpStorage::getNextSession(i64 *sessionId, time_t *timestamp) {
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
        *sessionId = sqlite3_column_int64(m_stmt, 0);
        *timestamp = sqlite3_column_int(m_stmt, 1);
    } else {
        assert(false);
    }

    return true;
}


void DumpStorage::initGetDownloads() {
    assert(false);
    /*
    int ret;
    
    if(m_stmt != NULL) {
        sqlite3_finalize(m_stmt);
        m_stmt = NULL;
    }

    ret = sqlite3_prepare(m_db, "select msgData, timestamp, dstId, ROWID from auditlog where ((srcId = ? and dstId = ?) or (srcId = ? and dstId = ?)) and msgType = ? order by ROWID asc", -1, &m_stmt, 0);
    assert(ret == SQLITE_OK);

    ret = sqlite3_bind_int(m_stmt, 1, UI_ID); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_int(m_stmt, 2, STORAGE_ID); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_int(m_stmt, 3, STORAGE_ID); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_int(m_stmt, 4, UI_ID); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_int(m_stmt, 5, MSG_RETRV_OBJECT); assert(ret == SQLITE_OK);    
    */
}

bool DumpStorage::getNextDownload(string *fileName, time_t *timestamp, i64 *rowId) {
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
        if(sqlite3_column_int(m_stmt, 2) == UI_ID) {
            *fileName = "RET " + *fileName;
        } else {
            *fileName = string((const char *) sqlite3_column_text(m_stmt, 0));
        }
        *timestamp = sqlite3_column_int(m_stmt, 1);
        *rowId = sqlite3_column_int64(m_stmt, 3);
    } else {
        assert(false);
    }

    return true;
}

void DumpStorage::initGetObjectAuditRecords(i64 rowId, bool reverse) {
    int ret;
    string queryString;    
    char idStr[128];

    if(m_stmt != NULL) {
        sqlite3_finalize(m_stmt);
        m_stmt = NULL;
    }

    if(rowId > 0) {
        sprintf(idStr, "%ld", rowId);
        queryString = "select * from auditlog where ROWID <= " + string(idStr);
    } else {
        queryString = "select * from auditlog";
    }
    if(reverse) {
        queryString += " order by ROWID desc";
    } else {
        queryString += " order by ROWID asc";
    }

    ret = sqlite3_prepare(m_db, queryString.c_str(), -1, &m_stmt, 0);
    if(ret != SQLITE_OK) {
        cerr << "logstoreage: coudl not init message ret = " << ret << endl;
        exit(1);
    }    
}

bool DumpStorage::getNextMessage(Message *msg, struct timeval *tv) {
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
        if(tv != NULL) {
            tv->tv_sec = sqlite3_column_int64(m_stmt, 7);
            tv->tv_usec = sqlite3_column_int64(m_stmt, 8);
        }
    } else {
        cerr << "error with getNextMessage" << endl;
        exit(1);
    }


    return true;
}

