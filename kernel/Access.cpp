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
#include "Access.h"
#include "kernel.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <utility>
#include <map>

AccessControlCheck::AccessControlCheck(i32 src, i32 dst)
    : m_src(src)
    , m_dst(dst)
{
}

bool AccessControlCheck::allowed(Message* msg)
{
    int type = msg->getMsgType();
    if (m_messages.find(type) == m_messages.end())
        return false;
    return true;
}

void AccessControlCheck::addMessage(int type)
{
    m_messages.insert(type);
}

i32 AccessControlCheck::getSrcId() const
{
    return m_src;
}

i32 AccessControlCheck::getDstId() const
{
    return m_dst;
}

AccessControl* AccessControl::_instance = NULL;

AccessControl::AccessControl()
    : m_procMap(NULL)
{
    // set WEBAPP to UI
    AccessControlCheck* w2u = new AccessControlCheck(WEBAPP_FIRST_ID, UI_ID);
    w2u->addMessage(MSG_UI_MSG);
    w2u->addMessage(MSG_NEW_WEBAPP);
    registerACC(w2u);
        
    // set WEBAPP to COOKIE
    AccessControlCheck* w2c = new AccessControlCheck(WEBAPP_FIRST_ID, COOKIE_ID);
    w2c->addMessage(MSG_DOM_COOKIE_SET);
    w2c->addMessage(MSG_DOM_COOKIE_GET);
    registerACC(w2c);

    // set WEBAPP to NETWORK
    AccessControlCheck* w2n = new AccessControlCheck(WEBAPP_FIRST_ID, NETWORK_ID);
    w2n->addMessage(MSG_FETCH_URL);
    w2n->addMessage(MSG_FETCH_URL_ABORT);
    registerACC(w2n);

    // set WEBAPP to STORAGE
    AccessControlCheck* w2s = new AccessControlCheck(WEBAPP_FIRST_ID, STORAGE_ID);
    w2s->addMessage(MSG_WRITE_FILE);
    w2s->addMessage(MSG_READ_FILE);
    registerACC(w2s);

    // set UI to WEBAPP
    AccessControlCheck* u2w = new AccessControlCheck(UI_ID, WEBAPP_FIRST_ID);
    u2w->addMessage(MSG_WEBAPP_MSG);
    registerACC(u2w);

    // set NETWORK to COOKIE
    AccessControlCheck* n2c = new AccessControlCheck(NETWORK_ID, COOKIE_ID);
    n2c->addMessage(MSG_COOKIE_SET);
    n2c->addMessage(MSG_COOKIE_GET);
    registerACC(n2c);

    // set NETWORK to WEBAPP
    AccessControlCheck* n2w = new AccessControlCheck(NETWORK_ID, WEBAPP_FIRST_ID);
    n2w->addMessage(MSG_RETURN_URL);
    n2w->addMessage(MSG_RETURN_URL_METADATA);
    registerACC(n2w);

    // set COOKIE to WEBAPP
    AccessControlCheck* c2w = new AccessControlCheck(COOKIE_ID, WEBAPP_FIRST_ID);
    c2w->addMessage(MSG_DOM_COOKIE_GET_RETURN);
    registerACC(c2w);

    // set COOKIE to NETWORK
    AccessControlCheck* c2n = new AccessControlCheck(COOKIE_ID, NETWORK_ID);
    c2n->addMessage(MSG_COOKIE_GET_RETURN);
    registerACC(c2n);

    // set STORAGE to WEBAPP
    AccessControlCheck* s2w = new AccessControlCheck(STORAGE_ID, WEBAPP_FIRST_ID);
    s2w->addMessage(MSG_READ_FILE_RETURN);
    registerACC(s2w);

    // set STORAGE to UI
    AccessControlCheck* s2u = new AccessControlCheck(STORAGE_ID, UI_ID);
    s2u->addMessage(MSG_DOWNLOAD_INFO);
    registerACC(s2u);
}

AccessControlCheck* AccessControl::getACC(i32 src, i32 dst)
{
    AccessControlCheck* chk = NULL;
    if (src >= WEBAPP_FIRST_ID)
        src = WEBAPP_FIRST_ID;
    if (dst >= WEBAPP_FIRST_ID)
        dst = WEBAPP_FIRST_ID;

    pair<i32, i32> lookup(src, dst);
    map<pair<i32, i32>, AccessControlCheck*>::iterator iter = m_acc.find(lookup);

    if(iter != m_acc.end()) {
        chk = iter->second; 
        return chk;
    }
    return NULL;
}

void AccessControl::registerACC(AccessControlCheck* ccptr)
{
    AccessControlCheck* chk = NULL;
    int src = ccptr->getSrcId();
    int dst = ccptr->getDstId();

    pair<i32, i32> lookup(src, dst);
    map<pair<i32, i32>, AccessControlCheck*>::iterator iter = m_acc.find(lookup);
    
    if(iter != m_acc.end()) {
        chk = iter->second;
        if (chk != NULL)
            delete chk;
    }
    m_acc[lookup] = ccptr;
}

bool AccessControl::checkWrite(Message* msg)
{
    i32 src = msg->getSrcId();
    i32 dst = msg->getDstId();
    if (src == KERNEL_ID || dst == KERNEL_ID) {
        // kernel can send message to any subsystem
        // any subsystem can send any message to kernel but kernel will handle it at its own discreation
        return true;
    }

    AccessControlCheck* chk = getACC(src, dst);
    if (chk != NULL)
        return chk->allowed(msg);
    return false;
}

bool AccessControl::checkRead(Message* msg)
{
    return true;
}

/*
bool AccessControlCheckPluginWebApp::allowed() { 
    //
    // If the domain set for the Plugin and the Domain setfor the webapp are 
    // the same, then we allow communication. If the domains are different, we 
    // deny.

    switch(_state) {
        case PLUGIN_IDLE: 
            if(_msg->getMsgType() == MSG_PLUGIN_SET_URL) {
                DomainName dn;
                dn.fromString(_msg->getStringData());
                AccessControl* ac = AccessControl::getInstance();
                ac->setLabel(_dst, dn);
                _state = PLUGIN_INIT;
                return true;
            }
            break;
        case PLUGIN_INIT:
            if(_msg->getMsgType() == MSG_PLUGIN_SET_URL) {
                DomainName dn;
                dn.fromString(_msg->getStringData());
                AccessControl* ac = AccessControl::getInstance();
                ac->setLabel(_dst, dn);
                return true;
            } else {
                AccessControl* ac = AccessControl::getInstance();
                bool ret = DomainName::sameDomainStrict(ac->getLabel(_src), ac->getLabel(_dst));
                if (!ret) fprintf(stderr, "domain name mismatch!!!!!!!!!\n");
                return ret;
            }
            break;
        default:
            break;
    }

    return false; 
};
*/

