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
#include "WebAppMgr.h"
#include "Message.h"
#include "DomainName.h"
#include "WebAppCache.h"

#include <iostream>
#include <stdio.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <map>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#define RESTORE_TIMEOUT 3
#define NO_MATCH (-1)

// This define is to disable pre creating web pages.  It should only be used
// if you are running performance numbers to quantify the impact of performance
// optimizations.  Normally this define should be commented out
//#define DONT_PRE_CREATE

struct WebAppProcess {
    Process_t *proc;
    i32 client;
    i32 container;

    WebAppProcess()
        : proc(0)
        , client(0)
        , container(0)
    {
    }
};

//static char *webAppArgv[] = {"strace", "-o", "strace.out","-f","-ff","./webapp/kde/browser", NULL};
static const char *webAppArgv[] = {"./webapp/op2/op2", NULL};

WebAppMgr::WebAppMgr(int uiFd)
{
    m_uiFd = uiFd;
    m_webAppIdOffset = 0;
    m_cachedWebApp = 0;
    if(!IS_CRAWLER) {
        m_cache = new WebAppCache();
    } else {
        m_cache = new WebAppCache(WebAppCache::SOP_Match, 0);
    }

    //int first
#if !defined(DONT_PRE_CREATE)
    preCreateWebApp();
#endif
}

void WebAppMgr::killChildren() {
    if (m_cachedWebApp) {
        if (m_cachedWebApp->proc) {
            kill(m_cachedWebApp->proc->pid, SIGINT);
        }
        // This is a memory leak but I don't think it matters since I am
        // assuming this is only going to get called when OP is about
        // to die
        m_cachedWebApp = 0;
    }
}

void WebAppMgr::handleMessage(Process_t *proc, Message *msg) {
    if (msg->getMsgType() == MSG_NEW_WEBAPP) {
        if (m_cachedWebApp !=0 && m_cachedWebApp->proc == proc) {
            assert(m_cachedWebApp->client == 0);
            m_cachedWebApp->client = msg->getMsgValue();
            m_cachedWebApp->proc->clientId = msg->getMsgValue();
        }
    }
}

void WebAppMgr::handleEmbedFrame(Process_t *proc, Message *msg) {    
    // need to keep track of "delegations" here for different processes and webapps
    msg->writeMessageKernel(m_uiFd);
}

WebAppProcess* WebAppMgr::prepNewProc(const i32 containerId,
                                      const i32 requester,
                                      const std::string& url) {

    WebAppProcess* ret;
    // we first try to look up in the cache
    Process_t* entry = m_cache->hit(url);
    if (entry != 0) {
        ret = new WebAppProcess;
        ret->proc = entry;
        ret->client = entry->clientId;
    } else { // if we cannot found a match in cache, use the pre-created one
        while (m_cachedWebApp == 0)
            preCreateWebApp();
        ret = m_cachedWebApp;
        m_cachedWebApp = 0;
    }

    addToWebAppList(ret);

    ret->container = containerId;

    if (requester >= WEBAPP_FIRST_ID) {
        int loadType = containerId;
        int webAppId = requester;
        map<i32, WebAppProcess *>::iterator iter = m_webAppList.find(webAppId);
        if (loadType == 1 && iter != m_webAppList.end()) {
            ret->container = iter->second->container;
        }
    }
    return ret;
}

void WebAppMgr::handleReplayWebApp(const i32 webAppId, const string& policies) {
    assert(m_webAppList.find(webAppId) == m_webAppList.end());

    while (m_cachedWebApp == 0)
        preCreateWebApp();

    // XXX FIXME: this is a bigtime hack to set the objectId this way
    i32 oldId = m_cachedWebApp->proc->objectId;
    m_cachedWebApp->proc->objectId = webAppId;

    addToWebAppList(m_cachedWebApp);
    m_cachedWebApp->container = 0;  
//    m_cachedWebApp->container = 1; // it's always 1 from nochrome-ui FIXME

    assert(m_webAppList.find(webAppId) != m_webAppList.end());

    m_cachedWebApp = NULL;

    replayWebApp(webAppId, policies);

    //FIXME
    //assert(false);
    WebAppProcess* app = m_webAppList[webAppId];
    Process_t* proc = app->proc;
    removeFromWebAppList(app);
    delete app;
    app = NULL;
    proc->objectId = oldId;
    removeFromProcMap(proc);
    delete proc;

}

void WebAppMgr::handleNewUrl(const i32 containerId, const i32 requester, i64& msgId,
                             const u8* data, const i32 len, const i32 urlId)
{
    bool subFrame = false;
    if (containerId == 0)
        return;
    
    if (containerId == 5) {
        subFrame = true;
    }
    
    DomainName dn;
    std::string url((const char*)data);
    dn.fromString(url);
    
    WebAppProcess* app = prepNewProc(containerId, requester, url);
    
    app->proc->label = dn;
    //app->proc->label.print();
    
    char str[33];
    sprintf(str, "%d", app->container);
    std::string container_str(str);

    
    // let the webapp know
    Message msg2;
    msg2.setSrcId(KERNEL_ID);
    msg2.setDstId(app->proc->objectId);
    msg2.setMsgId(msgId++);
    msg2.setMsgType(MSG_SET_URL);
    msg2.setMsgValue(urlId);
    msg2.setData(data, len);
    msg2.writeMessageKernel(app->proc->inFd[PIPE_WRITE]);
    // FIXME, after webapp receives this msg, it will show
    // so this will fix the problem of x11 (in mac & ubuntu)
    // but we definitely gamble here to assume that this msg
    // will arrive sooner than the one below (for embedding)
    
    
    // let the UI know that we have a new webApp
    // fake the src id
    
    // if the async creation hasn't get the WId, get it now
    // in most case, this is not possible
    if (app->client == 0) {
        Message* msg = new Message();
        msg->readMessageKernel(app->proc->outFd[PIPE_READ]);
        
        if (msg->getMsgType() != MSG_NEW_WEBAPP) {
            assert(0); // should not happen, init msg is always the first one
        } else {
            app->client = msg->getMsgValue();
            app->proc->clientId = msg->getMsgValue();
        }
        delete msg;
        
    }
    Message msg1(app->proc->objectId, UI_ID, msgId++, MSG_NEW_WEBAPP, app->client, container_str);
    msg1.writeMessageKernel(m_uiFd);
        
    if(!IS_CRAWLER) {
#if !defined(DONT_PRE_CREATE)
        preCreateWebApp();
#endif
    }
}

bool WebAppMgr::checkWebAppMsg(const i32 containerId, const i32 webAppId)
{
    map<i32, WebAppProcess *>::iterator iter = m_webAppList.find(webAppId);

    // no such webapp
    if (iter == m_webAppList.end()) {
        std::cerr << "container and client don't match" << std::endl;
        return false;
    }
    
    if (iter->second->container != containerId) {
        if (iter->second->container == 0) {
            iter->second->container = containerId;
            return true;
        }
        std::cerr << "container and client don't match" << std::endl;
        return false;
    } else {
        // contain and client match
        return true;
    }
}

void WebAppMgr::handleUnexpectedClose(const i32 webAppId) {
    WebAppProcess* app = m_webAppList[webAppId];
    removeFromProcMap(app->proc);
    delete app->proc;
    removeFromWebAppList(app);
    delete app;
}

void WebAppMgr::handleUpdateContainer(const i32 containerId, const i32 webAppId)
{
    map<i32, WebAppProcess *>::iterator iter = m_webAppList.find(webAppId);
    if (iter != m_webAppList.end()) {
        iter->second->container = containerId;
    }
}

void WebAppMgr::handleCloseWebApp(const i32 containerId, const i32 webAppId, i64& msgId)
{
    if (checkWebAppMsg(containerId, webAppId)) {

        Message msg;
        msg.setSrcId(KERNEL_ID);
        msg.setDstId(webAppId);
        msg.setMsgId(msgId++);
        msg.setMsgType(MSG_WEBAPP_MSG);
        msg.setMsgValue(MSG_WEBAPP_HIDE);
        msg.writeMessageKernel(m_webAppList[webAppId]->proc->inFd[PIPE_WRITE]);

        // put it into cache
        WebAppProcess* app = m_webAppList[webAppId];
        m_cache->store(app->proc, msgId);
        // clean up
        removeFromWebAppList(app);
        delete app;
        
    } else {
        cerr << "checkWebAppMsg check failed! " << containerId << " -> " << webAppId << endl;
    }
}

void WebAppMgr::preCreateWebApp() {
    if (m_cachedWebApp != 0)
        return;

    m_cachedWebApp = new WebAppProcess;
  
    i32 curWebAppId = m_webAppIdOffset + WEBAPP_FIRST_ID;
    m_webAppIdOffset ++;

    struct timeval start;
    gettimeofday(&start, NULL);
    
    m_cachedWebApp->proc = createProc(webAppArgv, curWebAppId);
    assert(m_cachedWebApp->proc != 0);

    addToProcMap(m_cachedWebApp->proc);
    
    struct timeval end;
    gettimeofday(&end, NULL);

    std::cerr << "creating cached webapp takes: "
	      << (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec)
	      << " microseconds" << std::endl;    
}

void WebAppMgr::addToWebAppList(WebAppProcess *webApp)
{
    if (webApp->proc) {
        assert(m_webAppList.find(webApp->proc->objectId) == m_webAppList.end());
        m_webAppList[webApp->proc->objectId] = webApp;
        //cerr << "ADD " << webApp->proc->objectId << "=" << webApp << endl;
    }
}

void WebAppMgr::removeFromWebAppList(WebAppProcess * webApp)
{
    assert(webApp != 0);
    assert(webApp->proc != 0);
    //cerr << "REMOVE " << webApp->proc->objectId << "=" << webApp << endl;
    map<i32, WebAppProcess *>::iterator iter = m_webAppList.find(webApp->proc->objectId);
    assert(iter != m_webAppList.end());
    assert(iter->second == webApp);
    m_webAppList.erase(iter);
}

void WebAppMgr::fillUrlIDMap(int fd, map<int, string> *fetchMap, map<int, string> * cookieMap, i64* ack) {
    Message appMsg;

    // check for any pending messages and read them if they are ready
    // make sure to keep track of any fetches we get along the way
    int bytesAvailable = 0;
    int ret = ioctl(fd, FIONREAD, &bytesAvailable);
    assert(ret == 0);
    while(bytesAvailable > 0) {
        appMsg.readMessageKernel(fd);
        if(appMsg.getMsgType() == MSG_FETCH_URL) {            
            if(fetchMap->find(appMsg.getMsgValue()) != fetchMap->end()) {
                cerr << "fetch with the same urlId " << appMsg.getMsgValue() << endl;
            }
            std::string url = appMsg.getStringData();
            (*fetchMap)[appMsg.getMsgValue()] = url;
            //std::cerr << "Replay URL: " << url
            //          << ", UID: " << appMsg.getMsgValue()
            //          << ", Type: " << appMsg.getMsgType() << std::endl;

        } else if (appMsg.getMsgType() == MSG_DOM_COOKIE_GET) {
            /*std::cerr << "Replay URL: " << appMsg.getStringData()
                      << ", UID: " << appMsg.getMsgValue()
                      << ", Type: " << appMsg.getMsgType() << std::endl;*/
            
            (*cookieMap)[appMsg.getMsgValue()] = appMsg.getStringData();
        } else if (appMsg.getMsgType() == MSG_ACK_REPLAY_MSG) {
            if (*ack > appMsg.getMsgValue()) {
                assert(0);
            }
            *ack = appMsg.getMsgValue();
        }
        ret = ioctl(fd, FIONREAD, &bytesAvailable);
        assert(ret == 0);
    }
}

bool WebAppMgr::replayWebApp(i32 webAppId, const string& policies) {
#if 0
    Message auditMsg;
    map<int, string> fetchMap;
    map<int, string> cookieMap;
    map<int, int> matchMap;
    i64 send = -1, ack = -1;
    bool ret = true;
    int fail_count = 0;
    Process_t *proc;

    assert(m_webAppList.find(webAppId) != m_webAppList.end());
    proc = m_webAppList[webAppId]->proc;
    assert(proc != NULL);

    size_t found = policies.find('\n', 0);
    size_t start = 0;

    string replayName = "";
    if (found != string::npos) {
        replayName = policies.substr(start, found - start);
    }
    start = found + 1;

    // tell the webapp, it's in replay model
    Message replayMsg(KERNEL_ID, webAppId, 0 /* we don't care about it*/, MSG_SET_REPLAY_MODE, 0, replayName);
    replayMsg.writeMessage(proc->inFd[PIPE_WRITE]);

    // set policies for replay
    found = policies.find('\n', start);
    while (found != string::npos) {
        string policy = policies.substr(start, found - start);
//        std::cerr << "Policy: " << policy << std::endl;
        Message policyMsg(KERNEL_ID, webAppId, 0 /* we don't care about it*/, MSG_SET_POLICY, 0, policy);
        policyMsg.writeMessage(proc->inFd[PIPE_WRITE]);
        start = found + 1;
        found = policies.find('\n', start);
    }
    if (policies.length() > start)  { // the last policy
        string policy = policies.substr(start, policies.length() - start);
        std::cerr << "Policy: " << policy << std::endl;
        Message policyMsg(KERNEL_ID, webAppId, 0 /* we don't care about it*/, MSG_SET_POLICY, 0, policy);
        policyMsg.writeMessage(proc->inFd[PIPE_WRITE]);
    }
    
    // replay audited messages
    auditMsg.getObjectAuditRecords(m_auditLogProc->inFd[PIPE_WRITE], webAppId);
    auditMsg.readMessage(m_auditLogProc->outFd[PIPE_READ]);
    while(auditMsg.getMsgType() != MSG_QUERY_AUDIT_LOG_REPLY) {
        if(auditMsg.getDstId() == webAppId) {
            if ((auditMsg.getMsgType() == MSG_WEBAPP_MSG)// && auditMsg.getMsgValue() != MSG_WEBAPP_CLOSE)
                || auditMsg.getMsgType() == MSG_SET_URL
                || auditMsg.getMsgType() == MSG_SAVE_DOM) {
                while (ack < send)
                    fillUrlIDMap(proc->outFd[PIPE_READ], &fetchMap, &cookieMap, &ack); 
                auditMsg.writeMessage(proc->inFd[PIPE_WRITE]);
            } else if (auditMsg.getMsgType() == MSG_DOM_COOKIE_GET_RETURN) {
                while (ack < send)
                    fillUrlIDMap(proc->outFd[PIPE_READ], &fetchMap, &cookieMap, &ack); 
                int newid = matchMap[auditMsg.getMsgValue()];
                if (newid != NO_MATCH) {
                    auditMsg.setMsgValue(newid);
                    auditMsg.writeMessage(proc->inFd[PIPE_WRITE]);
                    send = auditMsg.getMsgId();
                } else {
//                        cerr << "invalid uid/url couple, uid: " << auditMsg.getMsgValue() << endl;
                }

            } else if (auditMsg.getMsgType() == MSG_RETURN_URL ||
                       auditMsg.getMsgType() == MSG_RETURN_URL_METADATA ||
                       auditMsg.getMsgType() == MSG_UPDATE_URL) {
                
                if (matchMap.find(auditMsg.getMsgValue()) == matchMap.end()) {
                    // url id is correct
                    cerr << "WARNING: should not reach here!!!!!!" << endl;
                    auditMsg.writeMessage(proc->inFd[PIPE_WRITE]);
                } else {
                    int newid = matchMap[auditMsg.getMsgValue()];
                    if (newid != NO_MATCH) {
                        auditMsg.setMsgValue(newid);
                        auditMsg.writeMessage(proc->inFd[PIPE_WRITE]);
                        send = auditMsg.getMsgId();
                    } else {
//                        cerr << "invalid uid/url couple, uid: " << auditMsg.getMsgValue() << endl;
                    }
                }
            } else {
                cerr << "WARNING: should not reach here!!! type=" << auditMsg.getMsgType() << endl;
            }
        } else if (auditMsg.getMsgType() == MSG_LOG_USER_INPUT) {
            // replay user actions
            while (ack < send)
                fillUrlIDMap(proc->outFd[PIPE_READ], &fetchMap, &cookieMap, &ack); 
            auditMsg.writeMessage(proc->inFd[PIPE_WRITE]);
            send = auditMsg.getMsgId();
        } else if(((auditMsg.getMsgType() == MSG_FETCH_URL) || (auditMsg.getMsgType() == MSG_DOM_COOKIE_GET))
                  && (fail_count < 10)) {
            // make sure to sync up fetch/cookie requests so we do not
            // send any reply data before the webapp has asked for it
            int startTime = time(NULL);
            /*while((fetchMap.find(auditMsg.getMsgValue()) == fetchMap.end()
                   && cookieMap.find(auditMsg.getMsgValue()) == cookieMap.end())
                   || !match) {*/
            if (auditMsg.getMsgType() == MSG_FETCH_URL) {
                //cerr << "AUDITED URL: "<<auditMsg.getStringData() << ", UID: " << auditMsg.getMsgValue()
                //     << ", Type: " << auditMsg.getMsgType() << endl;
            }
            do{
                fillUrlIDMap(proc->outFd[PIPE_READ], &fetchMap, &cookieMap, &ack);
                if((time(NULL) - startTime) >= RESTORE_TIMEOUT && send == ack) {
                    cerr << "there was a problem, restore timed out looking for fetch, Type:  "
                         << auditMsg.getMsgType() << ", Url ID: "
                         << auditMsg.getMsgValue() << endl;// <<", Url:  " << auditMsg.getStringData() << endl;
                    ret = false;
                    fail_count ++;
                    break;
                }
            
                if (auditMsg.getMsgType() == MSG_FETCH_URL) {
                    // if(fetchMap[auditMsg.getMsgValue()] != auditMsg.getStringData()) {
                    matchMap[auditMsg.getMsgValue()] = NO_MATCH;
                    //cerr << "WARNING, fetch data mismatch " << fetchMap[auditMsg.getMsgValue()] <<
                    //    " <==> " << auditMsg.getStringData() << endl;
                    std::string url = auditMsg.getStringData();
                    if (url.find("POST:") == 0) {
                        url = url.substr(0,url.find("|"));
                    }

                    for (map<int, string>::iterator it = fetchMap.begin(); it != fetchMap.end(); it++) {
                        if (it->second == url) {
//                            cerr << "found match" << endl;
                            matchMap[auditMsg.getMsgValue()] = it->first;
                            fetchMap.erase(it);
                            break;
                        }
                    }
                    //} else {
                    //matchMap[auditMsg.getMsgValue()] = auditMsg.getMsgValue();
                    //}
                }
                if (auditMsg.getMsgType() == MSG_DOM_COOKIE_GET) {
                    //if (cookieMap[auditMsg.getMsgValue()] != auditMsg.getStringData()) {
                    matchMap[auditMsg.getMsgValue()] = NO_MATCH;
                    for (map<int, string>::iterator it = cookieMap.begin(); it != cookieMap.end(); it++) {
                        if (it->second == auditMsg.getStringData()) {
                            //cerr << "found match " << it->second << endl;
                            matchMap[auditMsg.getMsgValue()] = it->first;
                            cookieMap.erase(it);
                            break;
                        }
                    }
                    // } else {
                    //matchMap[auditMsg.getMsgValue()] = auditMsg.getMsgValue();
                    //}
                }

            } while (matchMap[auditMsg.getMsgValue()] == NO_MATCH);// && ret);
        }
        
        fillUrlIDMap(proc->outFd[PIPE_READ], &fetchMap, &cookieMap, &ack);
        
        auditMsg.readMessage(m_auditLogProc->outFd[PIPE_READ]);
    }
    
    cerr << "done restoring web app " << webAppId << endl;
    
    return ret;
#endif
    return true;
}

