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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <arpa/inet.h>
#include <assert.h>
#include <sqlite3.h>

#include "Message.h"

using namespace std;

#define LOG_STORE_FILE "auditlog.db"

const char *getMsgTypeStr(i32 msgType) {
    if(msgType == MSG_NEW_URL) {
        return "MSG_NEW_URL";
    } else if(msgType == MSG_FETCH_URL) {
        return "MSG_FETCH_URL";
    } else if(msgType == MSG_SET_URL) {
        return "MSG_SET_URL";
    } else if(msgType == MSG_NEW_WEBAPP) {
        return "MSG_NEW_WEBAPP";
    } else if(msgType == MSG_WEBAPP_MSG) {
        return "MSG_WEBAPP_MSG";
    } else if(msgType == MSG_loadStarted) {
        return "MSG_loadStarted";
    } else if(msgType == MSG_loadProgress) {
        return "MSG_loadProgress";
    } else if(msgType == MSG_COOKIE_GET) {
        return "MSG_COOKIE_GET";
    } else if(msgType == MSG_RETURN_URL_METADATA) {
        return "MSG_RETURN_URL_METADATA";
    } else if(msgType == MSG_RETURN_URL) {
        return "MSG_RETURN_URL";
    } else if(msgType == MSG_statusBarMessage) {
        return "MSG_statusBarMessage";
    } else if(msgType == MSG_titleChanged) {
        return "MSG_titleChanged";
    } else if(msgType == MSG_urlChanged) {
        return "MSG_urlChanged";
    } else if(msgType == MSG_loadFinished) {
        return "MSG_loadFinished";
    } else if(msgType == MSG_LOG_USER_INPUT) {
        return "MSG_LOG_USER_INPUT";
    }

    return "UNKNOWN";
}

int main(void) {
    struct MessageHeader header;
    unsigned char *data = NULL;
    int ret;
    sqlite3 *db;
    sqlite3_stmt *stmt;

    ret = sqlite3_open(LOG_STORE_FILE, &db);
    if( ret ) {
        cerr << "can't open database " << LOG_STORE_FILE << endl;
        db = NULL;
        return 0;
    }
    ret = sqlite3_prepare(db, "select * from auditlog order by ROWID asc", -1, &stmt, 0);
    assert(ret == SQLITE_OK);

    while((ret = sqlite3_step(stmt)) != SQLITE_DONE) {
        assert(ret == SQLITE_ROW);

        header.srcId = sqlite3_column_int(stmt, 0);
        header.dstId = sqlite3_column_int(stmt, 1);
        header.msgType = sqlite3_column_int(stmt, 3);
        header.msgValue = sqlite3_column_int(stmt, 4);
        header.dataLen = sqlite3_column_bytes(stmt, 6);

        //if(header.dstId == 1024 || header.srcId == 1024) {
        if(1) {
            cout << "reading message..." << endl;
            cout << "    header.srcId    = " << header.srcId << endl;
            cout << "    header.dstId    = " << header.dstId << endl;
            cout << "    header.msgType  = " << getMsgTypeStr(header.msgType) << " " << header.msgType << endl;
            cout << "    header.msgValue = " << header.msgValue << endl;
            cout << "    header.dataLen  = " << header.dataLen << endl;
        }

        if(data != NULL) {
            delete [] data;
            data = NULL;
        }
        
        if(header.dataLen > 0) {
            if(header.msgType == MSG_NEW_URL) {
                data = new unsigned char[header.dataLen+1];        
                memcpy(data, sqlite3_column_text(stmt, 6), header.dataLen);
                data[header.dataLen] = '\0';
                //cout << "    payload: " << (char *) data << endl;
            }
        }
    }
    
    return 0;
}
