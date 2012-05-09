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
#include <iomanip>
#include <string>
#include <assert.h>
#include <string.h>

#include <sys/time.h>

#include <map>

#include "DumpStorage.h"
#include "../kernel/kernel.h"

using namespace std;

map<i32, struct timeval> startTimeMap;
map<i32, struct timeval> endTimeMap;
map<i32, struct timeval> fetchTimeMap;
map<i32, string> urlMap;

void printTimeDiff(const char *msg, struct timeval start, struct timeval end) {
    struct timeval tv_diff;
    timersub(&end, &start, &tv_diff);
    cout << msg << tv_diff.tv_sec << "." << setw(6) << setfill('0') 
         << tv_diff.tv_usec << "s" << endl;
}

void dumpTimes() {
    i32 id;
    map<i32, string>::iterator iter;
    for(iter = urlMap.begin(); iter != urlMap.end(); iter++) {
        id = iter->first;
        cout << urlMap[id] << endl;
        //printTimeDiff("    startTime = ", startTimeMap[id], fetchTimeMap[id]);
        printTimeDiff("    loadTime  = ", startTimeMap[id], endTimeMap[id]);
    }

    startTimeMap.clear();
    endTimeMap.clear();
    urlMap.clear();
    fetchTimeMap.clear();
}

int main(int argc, char **argv){
    DumpStorage store;
    Message msg;
    i64 id;
    struct timeval tv;

    id = -1;

    store.initGetObjectAuditRecords(id, false);
    while(store.getNextMessage(&msg,&tv)) {
        if(msg.getMsgType() == MSG_SET_URL) {
            string url = msg.getStringData();
            if(url.size() > 45) {
                url = url.substr(0,45) + " ...";
            }            
            url = string(url.c_str());
            if(urlMap.find(msg.getDstId()) != urlMap.end()) {
                // if the dst id is repeated, we have a new session
                dumpTimes();
            }
            urlMap[msg.getDstId()] = url;
            startTimeMap[msg.getDstId()] = tv;
        } else if(msg.getMsgType() == MSG_urlChanged && (endTimeMap.find(msg.getSrcId()) != endTimeMap.end())) {
            // try to find the case when we reused the process for a new url
            // assume that the url is unchanged, this might be a bad assumption depending on
            // how the cache hit algorithm is configured
            string url = urlMap[msg.getSrcId()];
            dumpTimes();
            urlMap[msg.getSrcId()] = url;
            startTimeMap[msg.getSrcId()] = tv;
	} else if(msg.getMsgType() == MSG_loadStarted) {
            if(fetchTimeMap.find(msg.getSrcId()) == fetchTimeMap.end()) {
                fetchTimeMap[msg.getSrcId()] = tv;
            }
        } else if(msg.getMsgType() == MSG_loadFinished) {
            endTimeMap[msg.getSrcId()] = tv;
        }
    }

    dumpTimes();

    return 0;
}
