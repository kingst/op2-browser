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
#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <sqlite3.h>

#include "../kernel/Message.h"

using namespace std;

class Storage {

 public:
    Storage();
    ~Storage();

    int query(string query);

    void handleMessage(Message* m, int replyfd);

 private:
    sqlite3 *_db;
    sqlite3_stmt *m_readCookieStmt;

    int haveData; 

    // these might need to be threadsafe
    void writeFile(char* fname, void* contents);
    void readFile(char* fname);

    // these are for our object store
    void storeObject(i32 owner, string data);
    void retrvObject(i32 owner, string name, string *obj);
    void addAclUser(i32 user, i32 owner, string name);
    bool hasPermission(i32 user, i32 owner, string name);

    // these might need to be threadsafe
    int writeCookie(string contents);
    void initReadCookie(string fullstr);
    bool readNextCookie(string *cookie);
    int deleteCookie(string domain);
    void readDomains(string msg, string &domainlist);
    void numCookieJars(string &reply);
    void newCookieJar();
    void useCookieJar(string msg);
    int init();

    static int m_currentJar;
    static int m_numJars;
    static vector<pair<char *, char *> > lastResults;
    static const std::string StorageDB;

    // the right call back
    static int sqliteCallback(void *NotUsed, int argc, char **argv, char **azColName);
    // used for debugging
    static int sqlitePrintCallback(void *NotUsed, int argc, char **argv, char **azColName);
};
    
#endif
