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
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <string.h>
#include <time.h>

#include <map>

#include "Message.h"
#include "kernel.h"
#include "WebAppMgr.h"
#include "Access.h"
#include "DomainName.h"
#include "AIOHelper.h"

//static char *cacheArgv[] = {"network/pynetwork.py", NULL};
static const char *cacheArgv[] = {"network/network", NULL};
static const char *uiArgv[] = {"op-ui/op-ui", NULL};
static const char *uiCrawlerArgv[] = {"java", "-ea", "-jar", "build/jar/nochrome-ui.jar", NULL, NULL};
static const char *cookieArgv[] = {"cookie/cookie", NULL};
//static const char *auditArgv[] = {"log_storage/log_storage", NULL};
static const char *storageArgv[] = {"storage/storage", NULL};
//static char *pluginArgv[] = {"plugin/plugin", NULL};

using namespace std;

static WebAppMgr *webAppMgr;
static map<i32, Process_t *> procMap;
static map<pid_t, pair<i32, time_t> > killList;
static i64 msgId;
static i32 newUrlId;
static bool replayMode = false;

void addToProcMap(Process_t *proc) {
    assert(procMap.find(proc->objectId) == procMap.end());
    procMap[proc->objectId] = proc;
}

void removeFromProcMap(Process_t *proc) {
    map<i32, Process_t *>::iterator iter = procMap.find(proc->objectId);
    assert(iter != procMap.end());
    assert(iter->second == proc);
    procMap.erase(iter);
}

Process_t *createProc(const char *argv[], i32 objectId, const char *env) {
    int pret;
    Process_t *proc = new Process_t;
    const char *file = argv[0];

    pret = pipe(proc->inFd); assert(pret == 0);
    pret = pipe(proc->outFd); assert(pret == 0);

    proc->objectId = objectId;
    proc->pid = fork();
    if(proc->pid == 0) {
        dup2(proc->inFd[PIPE_READ], STDIN_FILENO);
        dup2(proc->outFd[PIPE_WRITE], STDOUT_FILENO);

        if(env != NULL) {
            putenv((char *) env);
        }

        execvp((char *) file, (char **) argv);
        assert(0);
    } else {
        // close up the unused file descriptors
        close(proc->inFd[PIPE_READ]);
        proc->inFd[PIPE_READ] = -1;
        close(proc->outFd[PIPE_WRITE]);        
        proc->outFd[PIPE_WRITE] = -1;
    }

    return proc;
}

void handleSyscall(Process_t *proc, Message *msg) {
    // dispatch table
    switch(msg->getMsgType()) {
    case MSG_NEW_URL:
        // src could be webapps
        // msgValue:
        //    0: not possible
        //    MSG_CUR_TAB 1:
        //    MSG_NEW_TAB_NO_SEL 2: NewTab without selection
        //    MSG_NEW_TAB_SEL 3: newTab with selection
        //    MSG_NEW_WINDOW 4: new window
        //    MSG_NEW_FRAME 5: new frame/iframe
        // msgValue
        //    contain winId:
        // format url + 0 + QVariant data
        if(!IS_CRAWLER) {
            webAppMgr->handleNewUrl(msg->getMsgValue(), msg->getSrcId(), msgId,
                                    msg->getMsgData(), msg->getDataLen(),
                                    newUrlId++);
        } else {
            if(msg->getSrcId() == UI_ID) {
                webAppMgr->handleNewUrl(msg->getMsgValue(), msg->getSrcId(), msgId,
                                        msg->getMsgData(), msg->getDataLen(),
                                        newUrlId++);
            } else {
                cerr << endl << "!!!!!!cannot open new url from non-ui!!!!!" << endl;
            }
        }
        break;
    case MSG_NEW_WEBAPP :
        // get the init msg with client WId
        webAppMgr->handleMessage(proc, msg);
        break;
    case MSG_UPDATA_CONTAINER: {
        std::string webAppId =  msg->getStringData();
        int id = atoi(webAppId.c_str());
        webAppMgr->handleUpdateContainer(msg->getMsgValue(), id);
        break;
    }
    case MSG_WEBAPP_CLOSE: {
        // from ui
        // MsgValue: containId
        std::string webAppId =  msg->getStringData();
        int id = atoi(webAppId.c_str());
        webAppMgr->handleCloseWebApp(msg->getMsgValue(), id, msgId);
        break;
    }
    case MSG_REPLAY_WEBAPP:
        // MsgValue: webAppId
        // MsgData: policies seperated by '\n'
        webAppMgr->handleReplayWebApp(msg->getMsgValue(), msg->getStringData());
        replayMode = true;
        break;
    case MSG_LOG_BEHAVIOR_DATA:
    case MSG_LOG_USER_INPUT:
        // these info have already been recorded
        break;
    case MSG_EMBED_FRAME:
        webAppMgr->handleEmbedFrame(proc, msg);
        break;
    default:
        cerr << "Unknown syscall msg type = " << msg->getMsgType() << " srcId = " << msg->getSrcId() << endl;
        assert(0);
    }
}

void addToKillList(Process_t *proc) {
    assert(killList.find(proc->pid) == killList.end());
    killList[proc->pid] = pair<i32, time_t>(proc->objectId, time(NULL));
}

void killProcs() {
    map<i32, Process_t *>::iterator iter;

    for(iter = procMap.begin(); iter != procMap.end(); iter++) {
        kill(iter->second->pid, SIGINT);
    }
    webAppMgr->killChildren();
    for(iter = procMap.begin(); iter != procMap.end(); iter++) {
        kill(iter->second->pid, SIGKILL);
    }
}

// system calls get handled by the kernel, other messages
// get routed to the appropriate component
void handleMessage(Process_t *proc, int fd) {
    Message msg;

    try {
        if (msg.hasDataAvailableKernel(fd)) {
            msg.readMessageKernel(fd, proc->objectId, msgId++);
        } else {
            // std::cerr << "No Message Available." << std::endl;
        }
    } catch (...) {
        if(proc->objectId < WEBAPP_FIRST_ID) {
            std::cerr << "kernel caught exception from process: "
                      << proc->objectId << std::endl;
            
            killProcs();
            exit(0);
        } else {
            //cerr << "webapp died, keep running..." << endl;
            return;
        }
    }

    Process_t *dstProc = NULL;
    if(msg.getDstId() == KERNEL_ID) {
        // system call
        handleSyscall(proc, &msg);    
        return;
    } else {
        map<i32, Process_t *>::iterator iter;
        iter = procMap.find(msg.getDstId());
        if(iter == procMap.end()) {
            fd = -1;
            //    std::cerr << "invalide no procMap entry for " << msg.getDstId() << std::endl;
        } else if(iter->second == NULL) {
            fd = -1;
            // std::cerr << "invalide null proc for " << msg.getDstId() << std::endl;
        } else {
            fd = iter->second->inFd[PIPE_WRITE];
            dstProc = iter->second;
        }
    }

    if(fd >= 0 && !replayMode) {
        msg.writeMessageKernel(fd);    
    }

}

static void setupFd(int fd, fd_set *fdSet, int *max) {
    if(fd > *max) {
        *max = fd;
    }

    FD_SET(fd, fdSet);
}

// I am assuming that we can't get recursive sigchld calls
void sigchldHandler(int signo) {
    pid_t pid;
    while( (pid = waitpid(-1, NULL, WNOHANG)) != 0) {
        if(killList.find(pid) != killList.end()) {
            //killList[proc->pid] = proc->objectId;
            Message msg2;
            msg2.setSrcId(KERNEL_ID);
            msg2.setDstId(UI_ID);
            msg2.setMsgId(msgId++);
            msg2.setMsgType(MSG_webAppExited);
            msg2.setMsgValue(killList[pid].first);
            msg2.writeMessageKernel(procMap[UI_ID]->inFd[PIPE_WRITE]);
            killList.erase(pid);
        } else {
            map<i32, Process_t *>::iterator iter;
            for(iter = procMap.begin(); iter != procMap.end(); iter++) {
                if(iter->second->pid == pid) {
                    if(iter->first >= WEBAPP_FIRST_ID) {
                        webAppMgr->handleUnexpectedClose(iter->first);
                    } else if(iter->first == UI_ID) {
                        killProcs();
                        exit(0);
                    } else {
                        cerr << "one of the non-ui and non webapp procs died, process " << iter->first << endl;
                        killProcs();
                        exit(0);
                    }
                    break;
                }
            }
        }
    }

    map<pid_t, pair<i32, time_t> >::iterator iter;
    for(iter = killList.begin(); iter != killList.end(); iter++) {
        if(iter->second.second > (time(NULL)-4)) {
            cerr << "found a lingering process, forcing kill" << endl;
            kill(iter->first, SIGKILL);
        }
    }
}

void sigintHandler(int signo) {
    std::cerr << "Got sigint, cleaning up" << std::endl;
    killProcs();
    exit(0);
}

int main(int argc, char*argv[]) {
    int maxFd, maxNonAppFd;    
    fd_set readSet;
    int ret;
    int idx;
    map<i32, Process_t *>::iterator iter;
    sigset_t mask, oldMask;

    msgId = 1;
    newUrlId = 1;

    signal(SIGINT, sigintHandler);
    signal(SIGCHLD, sigchldHandler);

    // access controls built in
    AccessControl::instance()->setProcMap(&procMap);

    addToProcMap(createProc(cacheArgv, NETWORK_ID));
    addToProcMap(createProc(cookieArgv, COOKIE_ID));
    addToProcMap(createProc(storageArgv, STORAGE_ID));
    if(IS_CRAWLER) {
        if (argc > 1)
            uiCrawlerArgv[4] = argv[1];
        addToProcMap(createProc(uiCrawlerArgv, UI_ID));
    } else {
        addToProcMap(createProc(uiArgv, UI_ID));
    }
    //addToProcMap(createProc(pluginArgv, PLUGIN_ID));
    //addToProcMap(createProc(jsArgv, JS_ID));

    webAppMgr = new WebAppMgr(procMap[UI_ID]->inFd[PIPE_WRITE]);

    // enter the main loop
    sigemptyset(&mask);
    sigaddset(&mask, ~SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldMask);

    while(1) {
        AIOHelper::instance()->gc(); // free buffers and control blocks used by aio_write
        
        maxFd = -1;
        FD_ZERO(&readSet);

        for(iter = procMap.begin(); iter != procMap.end(); iter++) {
            setupFd(iter->second->outFd[PIPE_READ], &readSet, &maxFd);
        }

        sigprocmask(SIG_SETMASK, &oldMask, NULL);

        ret = select(maxFd+1, &readSet, NULL, NULL, NULL);

        sigemptyset(&mask);
        sigaddset(&mask, ~SIGINT);
        sigprocmask(SIG_BLOCK, &mask, &oldMask);

        if(ret > 0) {
            // our iterator gets messed up if we handle more than one
            // message and the proc list is modified. Just call select
            // again
            // FIXME XXX: we should use round robin scheduling instead
            // of always choosing the begin element first...
            for(iter = procMap.begin(); iter != procMap.end(); iter++) {
                if(FD_ISSET(iter->second->outFd[PIPE_READ], &readSet)) {
                    handleMessage(iter->second, iter->second->outFd[PIPE_READ]);
                    break;
                }
            }
        }        
    }

    return 0;
}
