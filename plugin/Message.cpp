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
#include "Message.h"

#ifdef __OPKERNEL__
#include "Access.h"
#include "AIOHelper.h"
#endif

#include <iostream>

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <string>

#include <assert.h>

#include <sys/ioctl.h>

#ifdef __OPKERNEL__
//#define DUMP_READS
#endif

using namespace std;

// these are per process file descriptors that we setup on the first write
static int opWriteFd = -1;
static int opReadFd = STDIN_FILENO;

int Message::getReadFd() {
    assert(opReadFd >= 0);
    return opReadFd;
}

Message::Message() {
    this->msgData = NULL;
    memset(&header, 0, sizeof(header));
}

Message::Message(i32 srcId, i32 dstId, i64 msgId, i32 msgType, i32 msgValue, const string &data) {
    header.srcId = swapByteOrder(srcId);
    header.dstId = swapByteOrder(dstId);
    header.msgId = swapByteOrder(msgId);
    header.msgType = swapByteOrder(msgType);
    header.msgValue = swapByteOrder(msgValue);    
    header.dataLen = swapByteOrder((i32) data.size());

    if(data.size() > 0) {
        this->msgData = new u8[data.size()];
        memcpy(this->msgData, data.c_str(), data.size());
    } else {
        this->msgData = NULL;
    }
}

Message::Message(i32 srcId, i32 dstId, i64 msgId, i32 msgType, i32 msgValue, const void* data, i32 dataLen)
{
    header.srcId = swapByteOrder(srcId);
    header.dstId = swapByteOrder(dstId);
    header.msgId = swapByteOrder(msgId);
    header.msgType = swapByteOrder(msgType);
    header.msgValue = swapByteOrder(msgValue);    
    header.dataLen = swapByteOrder(dataLen);

    if(dataLen > 0) {
        this->msgData = new u8[dataLen];
        memcpy(this->msgData, data, dataLen);
    } else {
        this->msgData = NULL;
    }
}

Message::Message(const Message &) {
    assert(0);
}

Message *Message::copy() {
    Message *msg = new Message();
    msg->header = this->header;

    if(this->getDataLen() > 0) {
        msg->msgData = new u8[this->getDataLen()];
        memcpy(msg->msgData, this->msgData, this->getDataLen());
    }

    return msg;
}

Message::~Message() {
    if(this->msgData != NULL) {
        delete [] msgData;
    }
}

i32 Message::swapByteOrder(i32 in) {
    i32 ret;
    u8 *srcPtr, *dstPtr;

    dstPtr = (u8 *) &ret;
    srcPtr = (u8 *) &in;

    dstPtr[0] = srcPtr[3];
    dstPtr[1] = srcPtr[2];
    dstPtr[2] = srcPtr[1];
    dstPtr[3] = srcPtr[0];

    return ret;
}

i64 Message::swapByteOrder(i64 in) {
    i64 ret;
    u8 *srcPtr, *dstPtr;

    dstPtr = (u8 *) &ret;
    srcPtr = (u8 *) &in;

    dstPtr[0] = srcPtr[7];
    dstPtr[1] = srcPtr[6];
    dstPtr[2] = srcPtr[5];
    dstPtr[3] = srcPtr[4];
    dstPtr[4] = srcPtr[3];
    dstPtr[5] = srcPtr[2];
    dstPtr[6] = srcPtr[1];
    dstPtr[7] = srcPtr[0];

    return ret;
}

i32 Message::getMsgValue() {
    return swapByteOrder(header.msgValue);
}

i32 Message::getMsgType() {
    return swapByteOrder(header.msgType);
}

i32 Message::getSrcId() {
    return swapByteOrder(header.srcId);
}

i64 Message::getMsgId() {
    return swapByteOrder(header.msgId);
}

const u8 *Message::getMsgData() {
    return msgData;
}

void Message::setMsgType(i32 msgType) {
    header.msgType = swapByteOrder(msgType);
}
void Message::setMsgValue(i32 value) {
    header.msgValue = swapByteOrder(value);
}

void Message::setDstId(i32 dstId) {
    header.dstId = swapByteOrder(dstId);
}
i32 Message::getDstId() {
    return swapByteOrder(header.dstId);
}

i32 Message::getDataLen() {
    return swapByteOrder(header.dataLen);
}

void Message::setMsgId(i64 msgId) {
    header.msgId = swapByteOrder(msgId);
}

void Message::setSrcId(i32 srcId) {
    header.srcId = swapByteOrder(srcId);
}

void Message::setDataLen(i32 len) {
    header.dataLen = swapByteOrder(len);
}

int Message::readWriteBytes(int fd, void *buf, int numBytes, bool isWrite) {
   int bytesLeft, bytesHandled;
    int ret;
    u8 *tmpBuf = (u8 *) buf;

    bytesHandled = 0;
    do {
        bytesLeft = numBytes - bytesHandled;
        if(isWrite) {
            // XXX STK: I am not sure we need this since web page instances
            // can only communicate via cookies, which have their own
            // access controls built in
#ifdef __OPKERNEL__
            if (AccessControl::instance()->checkWrite(this)) {
                ret = AIOHelper::instance()->write(fd, tmpBuf + bytesHandled, bytesLeft);
            } else {
                std::cerr << getSrcId() << " -> " << getDstId() << " with " << getMsgType() << " not allowed" << std::endl;
                return numBytes;
            }
#else
            ret = write(fd, tmpBuf + bytesHandled, bytesLeft);
#endif
        } else {
            // XXX STK: I am not sure we need an access control check
            // here since web page instances can only communicate via
            // cookies, which have their own access controls built in
            ret = read(fd, tmpBuf + bytesHandled, bytesLeft);
        }
        bytesHandled += ret;
    } while((ret > 0) && (bytesHandled < numBytes));

    if(ret > 0) {
        assert(bytesHandled == numBytes);
    }

    return (ret > 0) ? bytesHandled : ret;
}

int Message::readBytes(int fd, void *buf, int numBytes) {
    return readWriteBytes(fd, buf, numBytes, false);
}

int Message::writeBytes(int fd, void *buf, int numBytes) {
    return readWriteBytes(fd, buf, numBytes, true);
}

bool Message::hasDataAvailable() {
    assert(opReadFd >= 0);
    return hasDataAvailableKernel(opReadFd);
}

bool Message::hasDataAvailableKernel(int fd) {
    ssize_t bytesAvailable = 0;
    int ret = ioctl(fd, FIONREAD, &bytesAvailable);
    assert(ret == 0);
    return bytesAvailable > 0;
}

void Message::readMessage() {
    assert(opReadFd >= 0);
    readMessageKernel(opReadFd, -1, -1);
}

void Message::readMessageKernel(int fd, i32 srcId, i64 msgId) {
    int ret;

    if(msgData != NULL) {
        delete [] msgData;
        msgData = NULL;
    }

    ret = readBytes(fd, &header, sizeof(header));
    if(ret == 0) {
        throw "Proc Exited\n";
    }
    assert(ret == sizeof(header));

    // if these values are to be set, they should be zero or positive
    if(msgId >= 0) {
        setMsgId(msgId);    
    }
    if(srcId >= 0) {
        setSrcId(srcId);
    }

#ifdef DUMP_READS    
    cout << "reading header from " << srcId << endl;
    cout.flush();
    writeBytes(STDOUT_FILENO, &header, sizeof(header));
    cout << "reading header return" << endl;
    cout.flush();
#endif

    if(getDataLen() > 0) {
        msgData = new u8[getDataLen()];
        ret = readBytes(fd, msgData, getDataLen());
        assert(ret == getDataLen());
#ifdef DUMP_READS
        cout << "reading payload size " << getDataLen() << " from " << srcId << endl;
        cout.flush();
	writeBytes(STDOUT_FILENO, msgData, getDataLen());
        cout << "reading payload done" << endl;
        cout.flush();
#endif
    }
}

void Message::llwriteMessage(int fd) {
    int ret;

    ret = writeBytes(fd, &header, sizeof(header));
    assert(ret == sizeof(header));

    if(getDataLen() > 0) {
        ret = writeBytes(fd, msgData, getDataLen());
        assert(ret == getDataLen());
    }
}

void Message::writeMessageKernel(int fd) {
    llwriteMessage(fd);
}
void Message::writeMessage() {
    if(opWriteFd < 0) {
        opWriteFd = dup(STDOUT_FILENO);
        assert(opWriteFd >= 0);
        int ret = dup2(STDERR_FILENO, STDOUT_FILENO);
        assert(ret == STDOUT_FILENO);
    }
    llwriteMessage(opWriteFd);
}

string Message::getStringData() {
    string ret;

    if(getDataLen() > 0) {
        assert(msgData != NULL);
        ret.append((char *) msgData, getDataLen());
    }

    return ret;
}

void Message::setData(const string &data) {
    setData(data.c_str(), data.size());
}

void Message::setData(const void *data, i32 dataLen) {
    if(msgData != NULL) {
        delete [] msgData;
        msgData = NULL;
    }

    setDataLen(dataLen);
    if(dataLen > 0) {
        msgData = new u8[dataLen];
        memcpy(msgData, data, dataLen);
    }

    assert(getDataLen() == dataLen);
}

void Message::setMsgData(i32 dstId, i32 msgType, i32 msgValue, string &data) {
    header.srcId = swapByteOrder(KERNEL_ID);
    header.dstId = swapByteOrder(dstId);
    header.msgType = swapByteOrder(msgType);
    header.msgValue = swapByteOrder(msgValue);

    setData(data);
}

