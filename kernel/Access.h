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
#ifndef __ACCESS_CONTROL_H__
#define __ACCESS_CONTROL_H__

#include <map>
#include <set>
#include <utility>

#include "Message.h"

using namespace std;

#ifdef __KERNEL_THREADS__
// in case we start using threads, dont forget to lock/unlock for the singleton!
class Mutex { 

    public: 
        Mutex() { pthread_mutex_init(&m, 0); }
        void lock() { pthread_mutex_lock(&m); }
        void unlock() { pthread_mutex_unlock(&m); }

    private:
        pthread_mutex_t m;
};
#endif

class AccessControlCheck {
 public:
    AccessControlCheck(i32 src, i32 dst);
    // for customized access control logic between two subsystems,
    // inherit this class and override the allowed() function
    virtual bool allowed(Message* msg);
    void addMessage(int type);
    i32 getSrcId() const;
    i32 getDstId() const;
    
 protected:
    std::set<int> m_messages;
    i32 m_src;
    i32 m_dst;
};


class AccessControl {

 public:
    bool checkWrite(Message* msg);
    bool checkRead(Message* msg);
    
    static AccessControl* instance() { 
        if(!_instance) {
            _instance = new AccessControl();
        }
        return _instance;
    };
    void registerACC(AccessControlCheck* ccptr);
    void setProcMap(map<i32, Process_t *>* ptr) { m_procMap = ptr; };
    
    inline DomainName getLabel(i32 id) { return ((*m_procMap)[id])->label; };
    inline void setLabel(i32 id, DomainName dn) { ((*m_procMap)[id])->label = dn; };
    
    inline static void log(char* msg) { 
        //fprintf(_instance->logfp, "ACCESS CONTROL: %s\n", msg); 
    };
    inline static void log(const char* msg) { 
        //fprintf(_instance->logfp, "ACCESS CONTROL: %s\n", msg); 
    };
    
 protected:
    AccessControl();
    
 private:
    AccessControlCheck* getACC(i32 src, i32 dst);
    
    FILE* logfp;
    map< pair< i32, i32 >, AccessControlCheck* > m_acc;
    map<i32, Process_t *>* m_procMap;
    static AccessControl* _instance;
};


/*
class AccessControlCheckPluginWebApp : public AccessControlCheck { 
    public:
        virtual bool allowed();
        AccessControlCheckPluginWebApp(i32 src, i32 dst, Message *msg) : _state(0), AccessControlCheckWebApp(src, dst, msg) { };

        virtual AccessControlCheck* dup(i32 src, i32 dst, Message *msg) {
            return new AccessControlCheckPluginWebApp(src, dst, msg);
        };
    private:
        int _state;

        static const int PLUGIN_IDLE = 0;
        static const int PLUGIN_INIT = 1;
};
*/

#endif
