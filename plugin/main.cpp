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
#include <string>
#include <assert.h>
#include <boost/regex.hpp>

#include "Message.h"
#include "../kernel/kernel.h"

using namespace std;
using namespace boost;

typedef struct PluginProcess { 
    char**args;
    char*display;
    pthread_t thread;

    i32 webappId;
    i32 pid;

    int inFd[2];
    int outFd[2];

} PluginProcess_t;

PluginProcess_t *createPluginProc(PluginProcess_t* proc, char *argv[], i32 objectId, char *env) {
    int pret;
    char *file = argv[0];

    pret = pipe(proc->inFd); assert(pret == 0);
    pret = pipe(proc->outFd); assert(pret == 0);

    proc->webappId = objectId;
    proc->pid = fork();
    if(proc->pid == 0) {
        dup2(proc->inFd[PIPE_READ], STDIN_FILENO);
        dup2(proc->outFd[PIPE_WRITE], STDOUT_FILENO);

        if(env != NULL) {
            putenv(env);
        }

        execvp(file, argv);
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

static void setupFd(int fd, fd_set *fdSet, int *max) {
    if(fd > *max) {
        *max = fd;
    }
    FD_SET(fd, fdSet);
}

int main(int argc, char **argv){

    int maxFd;
    fd_set readSet;
    int ret;

    vector<PluginProcess_t*> running_plugins;

    // enter the main loop
    while(1) {

        maxFd = -1;
        FD_ZERO(&readSet);

        FD_SET(STDIN_FILENO, &readSet);
        maxFd = 0;

        for(unsigned int x = 0; x < running_plugins.size(); x++) {
            setupFd(running_plugins[x]->outFd[PIPE_READ], &readSet, &maxFd);      
        }

        ret = select(maxFd+1, &readSet, NULL, NULL, NULL);

        if(ret == -1)
            assert(0);

        if(FD_ISSET(STDIN_FILENO, &readSet)) {

            Message m;
            m.readMessage(STDIN_FILENO);

            if(m.getMsgType() == MSG_PLUGIN_SET_URL) { 
                string msg = m.getStringData();
                string url, display, mime, wid;

                regex expression("^(.*);(.*);(.*);(.*)");

                cmatch what;
                if(regex_match(msg.c_str(), what, expression)) {
                    url.assign(what[1].first, what[1].second);
                    display.assign(what[2].first, what[2].second);
                    mime.assign(what[3].first, what[3].second);
                    wid.assign(what[4].first, what[4].second);
                } else {
                    cerr << "PluginManager: Plugin got invalid message format for SET_URL" << endl;
                    assert(false);
                }
                display = "DISPLAY=" + display;

                char* displaystr = strdup(display.c_str());
                char* urlstr = strdup(url.c_str());
                char* widstr = strdup(wid.c_str());

                cerr << "PluginManager: Plugin start request: URL=" << url << " on " << display << " mime=" << mime << " wid=" << wid << endl;

                if(!strcmp(mime.c_str(), "video/divx")) {
                    cerr << "PluginManager: Mplayer --- STARTING Mplayer on " << displaystr << " for URL " << urlstr << endl;
                    char *args[] = {"mplayer", "-fixed-vo", "-really-quiet", "-loop", "0", urlstr, NULL};

                    PluginProcess_t* tmp = new PluginProcess_t;
                    tmp->args = args;
                    tmp->display = displaystr;
                    tmp->webappId = m.getSrcId();

                    running_plugins.push_back(tmp);

                    createPluginProc(tmp, args, m.getSrcId(), displaystr);

#if 0 
                } else if(!strcmp(mime.c_str(), "application/x-shockwave-flash")) {
                    cerr << "FlashPlayer --- STARTING Flash Player on " << displaystr << " for URL " << urlstr << endl;
                    char *args[] = {"kde-gnash", urlstr, NULL};

                    PluginProcess_t tmp;
                    tmp.args = args;
                    tmp.display = displaystr;
                    startGnashThread((void*)&tmp);
                    //pthread_create(&tmp.thread, NULL, startGnashThread, (void*)&tmp);
                    running_plugins.push_back(tmp);
#endif
                } else if(!strcmp(mime.c_str(), "application/x-shockwave-flash")) {
                    cerr << "PluginManager: SAMPLE --- STARTING sample on " << displaystr << " for URL " << urlstr << endl;
                    char *args[] = {"./plugin/sample/sample", urlstr, NULL};

                    PluginProcess_t* tmp = new PluginProcess_t;
                    tmp->args = args;
                    tmp->display = displaystr;
                    tmp->webappId = m.getSrcId();

                    running_plugins.push_back(tmp);

                    createPluginProc(tmp, args, m.getSrcId(), displaystr);
                }
            } else {
                cerr << "PluginManager: we got a message TO one of the plugins!\n"; 
                cerr << "PluginManager: msg from " << m.getSrcId() << " with contents " << m.getStringData().c_str() << endl;
                for(unsigned int x = 0; x < running_plugins.size(); x++) {
                    if(running_plugins[x]->webappId == m.getSrcId()) {
                        cerr << "PluginManager: sending message to " << running_plugins[x]->args[0] << endl;
                        m.writeMessage(running_plugins[x]->inFd[PIPE_WRITE]);
                    }      
                }   
            }
        }
        else { 

            cerr << "PluginManager: we got a message FROM one of the plugins!\n"; 
            for(unsigned int x = 0; x < running_plugins.size(); x++) {
                if(FD_ISSET(running_plugins[x]->outFd[PIPE_READ], &readSet)) {

                    Message m;
                    m.readMessage(running_plugins[x]->outFd[PIPE_READ]);
                    m.writeMessage(STDOUT_FILENO);

                }      
            }
        }
    }
    return 0;
}
