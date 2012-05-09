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
#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#define MSG_TYPE_STRING 1

#include "kernel.h"
#include <string>

// this struct will have big endian byte order so you should never
// access values directly if you need to read or write something
// Make sure to go through the helper functions (e.g., getDataLen())
// that will do the appropriate byte swapping
struct MessageHeader {
    i32 srcId;
    i32 dstId;
    i64 msgId;
    i32 msgType;
    i32 msgValue;
    i32 dataLen;
} __attribute__((__packed__));

class Message {
 public:
    Message();
    Message(i32 srcId, i32 dstId, i64 msgId, i32 msgType, i32 msgValue, const std::string &data);
    Message(i32 srcId, i32 dstId, i64 msgId, i32 msgType, i32 msgValue, const void* data, i32 dataLen);
    Message(const Message& m);
    ~Message();
    Message *copy();

#ifdef __OPKERNEL__
    void readMessageKernel(int fd, i32 srcId = -1, i64 msgId = -1);
    void writeMessageKernel(int fd);
    bool hasDataAvailableKernel(int fd);
#else
    void readMessage();
    void writeMessage();
    bool hasDataAvailable();
    static int getReadFd();
#endif

    void setData(const void *data, i32 dataLen);
    void setData(const std::string &str);
    void setMsgValue(i32 value);
    void setDstId(i32 dstId);
    void setSrcId(i32 srcId);
    void setMsgId(i64 msgId);
    void setMsgType(i32 msgType);

    i32 getDstId();
    i32 getSrcId();
    i64 getMsgId();
    i32 getMsgType();
    i32 getMsgValue();
    i32 getDataLen();
    const u8 *getMsgData();
    void setMsgData(i32 dstId, i32 msgType, i32 msgValue, std::string &data);
    std::string getStringData();


 private:
    void llwriteMessage(int fd);
    int readBytes(int fd, void *buf, int numBytes);
    int writeBytes(int fd, void *buf, int numBytes);
    int readWriteBytes(int fd, void *buf, int numBytes, bool isWrite);
    i32 swapByteOrder(i32 in);
    i64 swapByteOrder(i64 in);
    void setDataLen(i32 len);

#ifdef __OPKERNEL__
    void readMessage();
    void writeMessage();
    bool hasDataAvailable();
    static int getReadFd();
#else
    void readMessageKernel(int fd, i32 srcId, i64 msgId);
    bool hasDataAvailableKernel(int fd);
    void writeMessageKernel(int fd);
#endif

    struct MessageHeader header;
    u8 *msgData;
};

#endif
