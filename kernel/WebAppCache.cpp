/*======================================================== 
**University of Illinois/NCSA 
**Open Source License 
**
OB**Copyright (C) 2007-2008,The Board of Trustees of the University of 
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

#include "WebAppCache.h"
#include "Message.h"
#include <assert.h>

using namespace std;

WebAppCache::WebAppCache(MatchPolicy mp, int size)
    : m_size(size),
      m_matchPolicy(mp)
{
}

Process_t* WebAppCache::hit(std::string url)
{
    Process_t* entry = 0;
    DomainName dn;
    dn.fromString(url);

    /*
    int count = 0;
    std::cerr << "For: " << url << std::endl;
    for (list<Process_t*>::iterator iter = m_entries.begin();
         iter != m_entries.end(); ++ iter) {
        std::cerr << count++ << " " << (*iter)->label.url << std::endl;
        }*/
    
    for (list<Process_t*>::iterator iter = m_entries.begin();
         iter != m_entries.end(); ++ iter) {
        
        if (m_matchPolicy == SOP_Match) {
            if (DomainName::sameOrigin((*iter)->label, dn)) {
                entry = *iter;
                m_entries.erase(iter);
                break;
            }
        } else if (m_matchPolicy == Exact_Match) {
            if ((*iter)->label.url == dn.url) {
                entry = *iter;
                m_entries.erase(iter);
                break;
            }
        } else {
            assert(false);
        }
    }
    return entry;
}

void WebAppCache::store(Process_t* entry, i64& msgId)
{
    // currently, we use LRU, i.e. push the entry to the back of the list
    m_entries.push_back(entry);
    if (m_entries.size() > m_size)
        evict(msgId);
}

void WebAppCache::evict(i64& msgId)
{
    // currently, we use LRU, i.e. remove the head of the list
    Process_t* entry = m_entries.front();
    m_entries.pop_front();

    Message msg;
    msg.setSrcId(KERNEL_ID);
    msg.setDstId(entry->objectId);
    msg.setMsgId(msgId++);
    msg.setMsgType(MSG_WEBAPP_MSG);
    msg.setMsgValue(MSG_WEBAPP_CLOSE);
    msg.writeMessageKernel(entry->inFd[PIPE_WRITE]);
    
    addToKillList(entry);
    removeFromProcMap(entry);
    delete entry;
}

