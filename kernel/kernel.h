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
#ifndef __WEBKERNEL__H_
#define __WEBKERNEL__H_

#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>

#include "DomainName.h"

typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;


typedef struct ProcessStruct {
    int inFd[2];
    int outFd[2];
    pid_t pid;
    i32 objectId;
    DomainName label;
    int clientId;
    ~ProcessStruct() {
    }
} Process_t;

Process_t *createProc(const char *argv[], i32 objectId, const char *env = NULL);
void addToProcMap(Process_t *proc);
void removeFromProcMap(Process_t *proc);
void addToKillList(Process_t *proc);

#define KERNEL_ID         0
//#define AUDITLOG_ID       1
#define NETWORK_ID        2
#define UI_ID             3
#define COOKIE_ID         4
#define COOKIE_REPLICA_ID 5
#define STORAGE_ID        6
#define PLUGIN_ID         9

#define REPLICA_WEBKIT_ID 10

// make sure that the webapp first id is always the highest value
// because the software will assume that any objectId's >= webappfirst
// belong to a web app
#define WEBAPP_FIRST_ID 1024

#define MSG_NEW_URL                    0
#define MSG_FETCH_URL                  1
#define MSG_RETURN_URL                 2
#define MSG_SET_URL                    3
#define MSG_UPDATE_URL                 4
#define MSG_UPDATA_CONTAINER           5
#define MSG_FETCH_URL_ABORT            6
#define MSG_SET_LOCATION_BAR           8
#define MSG_SET_STATUS_BAR             9
#define MSG_SET_CAPTION                10
#define MSG_FROM_VNC_CLIENT            11
#define MSG_FROM_VNC_SERVER            12
#define MSG_VNC_INIT                   13
#define MSG_NAV_SET_WEBAPP             14
#define MSG_NEW_WEBAPP                 15
#define MSG_NAV_STOP                   17
//#define MSG_GET_OBJECT_AUDIT_RECORDS   18
//#define MSG_QUERY_AUDIT_LOG_REPLY      19
#define MSG_EXTENSION_ACTION           20
#define MSG_STORE_OBJECT               21
#define MSG_RETRV_OBJECT               22
#define MSG_RETURN_URL_METADATA        23
#define MSG_OBJECT_ADD_ACL_USER        24
#define MSG_OBJECT_REM_ACL_USER        25
#define MSG_OBJECT_DOWNLOAD_READY      26
#define MSG_JS_EVALUATE                27
#define MSG_JS_REPLY                   28
#define MSG_JS_SET_HANDLER_CODE        29
#define MSG_JS_INVOKE_HANDLER          30
#define MSG_SNAP_SHOT                  31
#define MSG_REPLAY_WEBAPP              32
#define MSG_SET_POLICY                 33
#define MSG_LOG_BEHAVIOR_DATA          34
#define MSG_LOG_USER_INPUT             35
#define MSG_SET_REPLAY_MODE            36
#define MSG_SAVE_DOM                   37
#define MSG_ACK_REPLAY_MSG             38 
#define MSG_EMBED_FRAME                39

#define MSG_COOKIE_SET                 1007
#define MSG_COOKIE_REMOVE              1008
#define MSG_COOKIE_LISTDOMAINS         1009
#define MSG_COOKIE_GET                 1010
#define MSG_COOKIE_GET_RETURN          1011
#define MSG_COOKIE_NEWJAR              1012
#define MSG_COOKIE_NUMJARS             1013
#define MSG_COOKIE_USEJAR              1014


#define MSG_PLUGIN_NPN                 1020
#define MSG_PLUGIN_NPP                 1021
#define MSG_PLUGIN_EXECUTE             1022
#define MSG_PLUGIN_SET_URL             1023

#define MSG_DOM_COOKIE_SET             1030
#define MSG_DOM_COOKIE_GET             1031
#define MSG_DOM_COOKIE_GET_RETURN      1032

#define MSG_WRITE_FILE                 1040
#define MSG_READ_FILE                  1041
#define MSG_READ_FILE_RETURN           1042
#define MSG_DOWNLOAD_INFO              1043

#define MSG_WEBAPP_MSG                 4000
enum WEBAPP_MSG {
    MSG_WEBAPP_CLOSE = MSG_WEBAPP_MSG,
    MSG_WEBAPP_SHOW,
    MSG_WEBAPP_HIDE,
    MSG_WEBAPP_LOAD_URL,
};

#define MSG_UI_MSG                     5000

enum UI_MSG {
    MSG_loadStarted = MSG_UI_MSG,
    MSG_loadProgress,
    MSG_loadFinished,
    MSG_linkHovered,
    MSG_statusBarMessage,
    MSG_geometryChangeRequested,
    MSG_windowCloseRequested,
    MSG_toolBarVisibilityChangeRequested,
    MSG_statusBarVisibilityChangeRequested,
    MSG_menuBarVisibilityChangeRequested,
    MSG_titleChanged,
    MSG_iconChanged,
    MSG_urlChanged,
    MSG_addHistoryItem,
    MSG_navBackOrForward,
    MSG_webAppExited,
};

#define PIPE_READ 0
#define PIPE_WRITE 1

#define IS_CRAWLER ((getenv("OP_CRAWLER") != NULL) && (strcmp(getenv("OP_CRAWLER"),"YES") == 0))
#define IFRAME_HANDLED ((getenv("OP_IFRAME") != NULL) && (strcmp(getenv("OP_IFRAME"),"YES") == 0))

#endif
