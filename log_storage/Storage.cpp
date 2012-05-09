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
#include "Cookie.h"
#include "../kernel/Message.h"

#include <iostream>
#include <signal.h>
#include <string>
#include <sstream>
#include <sqlite3.h>

#include <errno.h>
#include <assert.h>

using namespace std;

// FIXME: logging to stderr needs to go somewhere else!

const std::string Storage::StorageDB = "cookies.db";
vector<pair<char *, char*> > Storage::lastResults;
int Storage::m_currentJar = 0;
int Storage::m_numJars = 1;

/*
#define MSG_COOKIE_SET              1007
#define MSG_COOKIE_REMOVE           1008
#define MSG_COOKIE_LISTDOMAINS      1009
#define MSG_COOKIE_GET              1010
#define MSG_COOKIE_NEWJAR           1011
#define MSG_COOKIE_NUMJARS          1012
#define MSG_COOKIE_USEJAR           1013
*/
void Storage::handleMessage(Message* m, int replyfd) { 

    if(m->getMsgType() == MSG_COOKIE_SET) { 
        int ret = writeCookie(m->getStringData());
    } else if (m->getMsgType() == MSG_COOKIE_LISTDOMAINS) {
        string contents;
        readDomains(m->getStringData(), contents);
        
        Message reply(COOKIE_ID, CACHE_ID, 0, MSG_COOKIE_LISTDOMAINS, m->getMsgValue(), contents);
        reply.writeMessage(replyfd);
    } else if (m->getMsgType() == MSG_COOKIE_NEWJAR) {
        newCookieJar(); 
    } else if (m->getMsgType() == MSG_COOKIE_NUMJARS) {
        string contents;
        numCookieJars(contents);

        Message reply(COOKIE_ID, m->getSrcId(), 0, MSG_COOKIE_NUMJARS, m->getMsgValue(), contents);
        reply.writeMessage(replyfd);
    } else if (m->getMsgType() == MSG_COOKIE_USEJAR) {
        useCookieJar(m->getStringData());
    } else if(m->getMsgType() == MSG_COOKIE_GET) {
        string contents;
        initReadCookie(m->getStringData());
        while(readNextCookie(&contents))
            ;
        Message reply(COOKIE_ID, m->getSrcId(), 0, MSG_COOKIE_GET, m->getMsgValue(), contents);
        reply.writeMessage(replyfd);
    } else if(m->getMsgType() == MSG_COOKIE_REMOVE) {
        int ret = deleteCookie(m->getStringData());
    } else if(m->getMsgType() == MSG_STORE_OBJECT) {
        storeObject(m->getSrcId(), m->getStringData());
    } else if(m->getMsgType() == MSG_RETRV_OBJECT) {
        string object;
        retrvObject(m->getSrcId(), m->getStringData(), &object);

        Message reply(STORAGE_ID, m->getSrcId(), 0, MSG_RETRV_OBJECT, m->getMsgValue(), object);
        reply.writeMessage(replyfd);
    } else if(m->getMsgType() == MSG_OBJECT_ADD_ACL_USER) {
        addAclUser(m->getMsgValue(), m->getSrcId(), m->getStringData());
    }
}

void Storage::addAclUser(i32 user, i32 owner, string name) {
    int ret;
    sqlite3_stmt *stmt;
    
    ret = sqlite3_prepare(_db, "insert into object_acl values (?, ?, ?)", -1, &stmt, 0);
    assert(ret == SQLITE_OK);

    ret = sqlite3_bind_int(stmt, 1, user); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_int(stmt, 2, owner); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_text(stmt, 3, name.c_str(), name.size(), SQLITE_TRANSIENT);
    assert(ret == SQLITE_OK);

    ret = sqlite3_step(stmt);
    assert(ret == SQLITE_DONE);

    sqlite3_finalize(stmt);
}

bool Storage::hasPermission(i32 user, i32 owner, string name) {
    int ret;
    sqlite3_stmt *stmt;

    ret = sqlite3_prepare(_db, "select count(*) from object_acl where (user = ? and owner = ? and name = ?)",
                          -1, &stmt, 0);
    assert(ret == SQLITE_OK);

    ret = sqlite3_bind_int(stmt, 1, user); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_int(stmt, 2, owner); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_text(stmt, 3, name.c_str(), name.size(), SQLITE_TRANSIENT);
    assert(ret == SQLITE_OK);

    ret = sqlite3_step(stmt);
    assert(ret == SQLITE_ROW);

    ret = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);    
    assert((ret == 0) || (ret == 1));

    return ret > 0;
}

void Storage::retrvObject(i32 owner, string name, string *obj) {
    sqlite3_stmt *stmt;
    int ret;
    char tmp[64];
    string ownerStr;
    i32 user;

    // check if trying to read a non-owned object
    if(name.find(':') != string::npos) {
        user = owner;
        ownerStr = name.substr(name.find(':')+1);
        name = name.substr(0, name.find(':'));
        assert(ownerStr.size() > 0);
        assert(sizeof(int) == sizeof(i32));
        ret = sscanf(ownerStr.c_str(), "%d", &owner); assert(ret == 1);

        if(!hasPermission(user, owner, name)) {
            cerr << "error: tried to access object without permission" << endl;
            return;
        }
    }

    sprintf(tmp, "%d", owner);
    ownerStr = string(tmp);

    ret = sqlite3_prepare(_db, "select data from object_store where (owner = ? and name = ?)", -1, &stmt, 0);
    assert(ret == SQLITE_OK);

    ret = sqlite3_bind_int(stmt, 1, owner); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_text(stmt, 2, name.c_str(), name.length(), SQLITE_TRANSIENT); assert(ret == SQLITE_OK);

    ret = sqlite3_step(stmt);
    assert((ret == SQLITE_ROW) || (ret == SQLITE_DONE));

    if(ret == SQLITE_ROW) {        
        obj->append(name + string(":") + ownerStr + string(":"));
        obj->append((const char *) sqlite3_column_blob(stmt, 0),
                    sqlite3_column_bytes(stmt, 0));
    }
    sqlite3_finalize(stmt);
}

void Storage::storeObject(i32 owner, string data) {
    unsigned long loc = data.find(':');
    sqlite3_stmt *stmt;
    int ret;

    assert(loc != string::npos);
    assert(loc < (data.size()-1)); // no empty objects

    string name = data.substr(0, loc);
    string obj = data.substr(loc+1);

    // make sure we clear out the acl for any old objects with the
    // same name
    ret = sqlite3_prepare(_db, "delete from object_acl where (owner = ? and name = ?)", 
                          -1, &stmt, 0); assert(ret == SQLITE_OK);

    ret = sqlite3_bind_int(stmt, 1, owner); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_text(stmt, 2, name.c_str(), name.length(), SQLITE_TRANSIENT);
    assert(ret == SQLITE_OK);
    
    ret = sqlite3_step(stmt); assert(ret == SQLITE_DONE);
    sqlite3_finalize(stmt);

    // now store the new object
    ret = sqlite3_prepare(_db, "insert into object_store values (?, ?, ?)", -1, &stmt, 0);
    assert(ret == SQLITE_OK);

    ret = sqlite3_bind_int(stmt, 1, owner); assert(ret == SQLITE_OK);
    ret = sqlite3_bind_text(stmt, 2, name.c_str(), name.length(), SQLITE_TRANSIENT);
    ret = sqlite3_bind_blob(stmt, 3, obj.c_str(), obj.length(), SQLITE_TRANSIENT);

    ret = sqlite3_step(stmt); assert(ret == SQLITE_DONE);
    sqlite3_finalize(stmt);
}

void Storage::readDomains(string msg, string &domainlist) {
    sqlite3_stmt *stmt;
    int ret;

    ret = sqlite3_prepare(_db, 
            "select domain from cookies group by domain", -1, 
            &stmt, 0);

    if(ret != SQLITE_OK) {
        cerr << "storage::readDomains could not prepare errorcode " << ret << endl;
        assert(false);
        return;
    }

    while((ret = sqlite3_step(stmt)) == SQLITE_ROW) { 
        domainlist += (char*) sqlite3_column_text(stmt, 0);
        domainlist += ";";
    }

    if (ret != SQLITE_DONE && ret != SQLITE_ROW) {
        cerr << "storage::readDomains could not step (execute) stmt errorcode " << ret << endl;  
        sqlite3_finalize(stmt);
        assert(false);
        return;
    } else if (ret == SQLITE_DONE) { 
        sqlite3_finalize(stmt);
        return;
    } else if (ret == SQLITE_ROW) {
        cerr << "storage::readDoains got another row it was not expecting\n";
        assert(false);
        return;
    } 
}

void Storage::numCookieJars(string &reply) {
    ostringstream ostr;
    ostr << m_numJars;
    reply = ostr.str();
}

void Storage::newCookieJar() { 
    m_numJars++;
}

void Storage::useCookieJar(string msg) {
    int newJar = atoi(msg.c_str());

    if(newJar < m_numJars)
        m_currentJar = newJar;
    else
        cerr << "storage::useCookieJar tried to set an invalid jar number\n";
}

// return: 1 indicates error, 0 is ok
int Storage::deleteCookie(string contents) { 

    sqlite3_stmt *stmt;
    int ret;

    Cookie c;
    c.fillCookie(contents);

    //cerr << "storage::deleteCookie deleting domain=" << c.getDomain() << " name=" << c[4]
    //    << " secure=" << c[2] << endl;

    ret = sqlite3_prepare(_db, "delete from cookies where (domain = ? and name = ? and path = ?)", -1, &stmt, 0);
    if(ret != SQLITE_OK) {
        cerr << "storage::deleteCookie could not prepare errorcode " << ret << endl;
        return 1;
    }

    ret = sqlite3_bind_text(stmt, 1, c.getDomain().c_str(), c.getDomain().length(), SQLITE_TRANSIENT);
    if(ret != SQLITE_OK) {
        cerr << "storage::deleteCookie could not bind text (domain)";
        sqlite3_finalize(stmt);
        return 1;
    }
    ret = sqlite3_bind_text(stmt, 2, c[4].c_str(), c[4].length(), SQLITE_TRANSIENT);
    if(ret != SQLITE_OK) {
        cerr << "storage::deleteCookie could not bind text (name)";
        sqlite3_finalize(stmt);
        return 1;
    }
    
    ret = sqlite3_bind_text(stmt, 3, c[1].c_str(), c[1].length(), SQLITE_TRANSIENT);
    if(ret != SQLITE_OK) {
        cerr << "storage::deleteCookie could not bind text (secure)";
        sqlite3_finalize(stmt);
        return 1;
    }

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        //cerr << "storage::deleteCookie could not step (execute) stmt errorcode " << ret << endl;  
        sqlite3_finalize(stmt);
        return 1;
    }

    sqlite3_finalize(stmt);
    return 0;

}

//(domain text, flag text, path text, secure text, expiration text, name text, value text)

void Storage::initReadCookie(string domain) {
    if(m_readCookieStmt != NULL) {
        sqlite3_finalize(m_readCookieStmt);
        m_readCookieStmt = NULL;
    }

    //void Cookie::fillCookie(char * d, char* f, char* p, char* s, char* t, char* n, char* v)
    int ret = sqlite3_prepare(_db, 
            "select domain, flag, path, secure, expiration, name, value, creation from cookies where (domain = ? AND jar = ?)", -1, 
            &m_readCookieStmt, 0);

    if(ret != SQLITE_OK) {
        cerr << "storage::readCookie could not prepare errorcode " << ret << endl;
        assert(false);
        return;
    }

    ret = sqlite3_bind_text(m_readCookieStmt, 1, domain.c_str(), domain.length(), SQLITE_TRANSIENT);
    if(ret != SQLITE_OK) {
        cerr << "storage::readCookie could not bind text (domain)\n";
        sqlite3_finalize(m_readCookieStmt);
        assert(false);
        return;
    }

    ret = sqlite3_bind_int(m_readCookieStmt, 2, m_currentJar);
    if(ret != SQLITE_OK) {
        cerr << "storage::readCookie coudl not bind int (jar)\n";
        sqlite3_finalize(m_readCookieStmt);
        assert(false);
        return;
    } 
}

bool Storage::readNextCookie(string *cookie) {
    if(m_readCookieStmt == NULL) {
        return false;
    }

    int ret = sqlite3_step(m_readCookieStmt);
    if (ret != SQLITE_DONE && ret != SQLITE_ROW) {
        cerr << "storage::readCookie could not step (execute) stmt errorcode " << ret << endl;  
        sqlite3_finalize(m_readCookieStmt);
        assert(false);
        return false;
    }

    if(ret == SQLITE_ROW) {
        Cookie c;
        c.fillCookie(
                (char*) sqlite3_column_text(m_readCookieStmt, 0), 
                (char*) sqlite3_column_text(m_readCookieStmt, 1),
                (char*) sqlite3_column_text(m_readCookieStmt, 2),
                (char*) sqlite3_column_text(m_readCookieStmt, 3),
                (char*) sqlite3_column_text(m_readCookieStmt, 4),
                (char*) sqlite3_column_text(m_readCookieStmt, 5),
                (char*) sqlite3_column_text(m_readCookieStmt, 6),
                (char*) sqlite3_column_text(m_readCookieStmt, 7));
        *cookie += c.toString();
        return true;

    } else if (ret == SQLITE_DONE) { 
        sqlite3_finalize(m_readCookieStmt);
        m_readCookieStmt = NULL;
        return false;
    } else {
        cerr << "storage::readCookie had something bad happen!\n";
        assert(false);
        return false;
    }
}

// return: empty string on error or not found, cookie as string if ok
#if 0
string Storage::readCookie(string domain, string name) {

    string serialized;
    sqlite3_stmt *stmt;
    int ret;

    ret = sqlite3_prepare(_db, "select * from cookies where (domain = ?)", -1, &stmt, 0);
    if(ret != SQLITE_OK) {
        //cerr << "storage::readCookie could not prepare errorcode " << ret << endl;
        return "";
    }

    ret = sqlite3_bind_text(stmt, 1, domain.c_str(), domain.length(), SQLITE_TRANSIENT);
    if(ret != SQLITE_OK) {
        //cerr << "storage::readCookie could not bind text (domain)";
        sqlite3_finalize(stmt);
        return "";
    }
    /*
    ret = sqlite3_bind_text(stmt, 2, name.c_str(), name.length(), SQLITE_TRANSIENT);
    if(ret != SQLITE_OK) {
        //cerr << "storage::readCookie could not bind text (name)";
        sqlite3_finalize(stmt);
        return "";
    }
    */
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE && ret != SQLITE_ROW) {
        //cerr << "storage::readCookie could not step (execute) stmt errorcode " << ret << endl;  
        sqlite3_finalize(stmt);
        return "";
    }

    if(ret == SQLITE_ROW) {
        //const unsigned char *sqlite3_column_text(sqlite3_stmt*, int iCol);
        Cookie c;
        c.fillCookie(
                (char*) sqlite3_column_text(stmt, 0), 
                (char*) sqlite3_column_text(stmt, 1),
                (char*) sqlite3_column_text(stmt, 2),
                (char*) sqlite3_column_text(stmt, 3),
                (char*) sqlite3_column_text(stmt, 4),
                (char*) sqlite3_column_text(stmt, 5),
                (char*) sqlite3_column_text(stmt, 6));
        sqlite3_finalize(stmt);
        return c.toString();

    } else if (ret == SQLITE_DONE) { 
        //cerr << "storage::readCookie didn't find any cookies!\n"; 
        Cookie c;
        c.createBlank();

        sqlite3_finalize(stmt);
        return c.toString();
    } else {
        cerr << "storage::readCookie had something bad happen!\n";

        sqlite3_finalize(stmt);
        return "";
    }


}
#endif

// return: 1 indicates error, 0 is ok
int Storage::writeCookie(string contents) {

    sqlite3_stmt *stmt;
    int ret;
    Cookie c;

    c.fillCookie(contents);

    string domain = c.getDomain();

    //cerr << "storage::writeCookie got " << contents << "\n";

    // (domain text, flag text, path text, secure text, expiration text, name text, value text, jar integer)
    ret = sqlite3_prepare(_db,
                          "insert into cookies (domain, flag, path, secure, expiration, name, value, creation, jar) values (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                          -1, &stmt, 0);
    if(ret != SQLITE_OK) {
        cerr << "storage::writeCookie prepare statement ret = " << ret << endl;
        return 1;
    }
    //int sqlite3_bind_text(sqlite3_stmt*, int, const char*, int n, void(*)(void*));
    ret = sqlite3_bind_text(stmt, 1, domain.c_str(), domain.length(), SQLITE_TRANSIENT);
    if(ret != SQLITE_OK) {
        cerr << "storage::writeCookie could not bind text ret = " << ret << endl;
        sqlite3_finalize(stmt);
        return 1;
    }

    for(int x = 0; x < c.size(); x++) {
        sqlite3_bind_text(stmt, x+2, c[x].c_str(), c[x].length(), SQLITE_TRANSIENT);
        if(ret != SQLITE_OK) {
            cerr << "storage::writeCookie error while binding - " << c[x] << endl;
            cerr << "storage::writeCookie could not bind text\n";
            sqlite3_finalize(stmt);
            return 1;
        }
    }

    assert(c.size() == 7);
    ret = sqlite3_bind_int(stmt, 9, m_currentJar);
    if(ret != SQLITE_OK) {
        cerr << "storage::writeCookie could not bind int (jar) " << ret << endl;
        sqlite3_finalize(stmt);
        return 1;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        cerr << ("storage::writeCookie could not step (execute) stmt.\n");
        sqlite3_finalize(stmt);
        return 1;
    }

    //cerr << "storage::writeCookie set cookie for domain=" << domain.c_str() << endl;

    sqlite3_finalize(stmt);
    return 0;
}

int Storage::sqlitePrintCallback(void *NotUsed, int argc, char **argv, char **azColName){
    for(int i=0; i<argc; i++){
        fprintf(stderr, "%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    fprintf(stderr, "\n");
    return 0;
}

int Storage::sqliteCallback(void *NotUsed, int argc, char **argv, char **azColName){

    lastResults.clear();

    for(int i=0; i<argc; i++){
        lastResults.push_back(pair<char *, char *>(azColName[i], argv[i]));
    }

    return 0;
}

int Storage::query(string query) {

    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_exec(_db, query.c_str(), sqlitePrintCallback, 0, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        fflush(stderr);
        sqlite3_free(zErrMsg);
        assert(false);
        return -1;
    }
    return 0;
}

int Storage::init() {
    int rc; 
    char idStr[64];

    // open the database
    rc = sqlite3_open(StorageDB.c_str(), &_db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(_db));
        return -1;
    }

    // now fill the colum names
    query("create table if not exists cookies (domain text, flag text, path text, secure text, expiration text, name text, value text, creation text, jar integer, primary key (domain, name, path) on conflict replace)");

    query("create table if not exists object_store (owner integer, name text, data blob, primary key(owner, name) on conflict replace)");

    query("create table if not exists object_acl (user integer, owner integer, name text)");

    query("PRAGMA synchronous=OFF");

    // clear out any webapp objects
    sprintf(idStr, "%d", WEBAPP_FIRST_ID);
    query("delete from object_store where owner >= " + string(idStr));
    query("delete from object_acl where owner >= " + string(idStr) + 
          " or user >= " + string(idStr));

    m_readCookieStmt = NULL;
    return 0;
}

Storage::~Storage() {
    sqlite3_close(_db);
    
    if(m_readCookieStmt != NULL) {
        sqlite3_finalize(m_readCookieStmt);
        m_readCookieStmt = NULL;
    }
}

Storage::Storage() : haveData(0) {
    init();
}
