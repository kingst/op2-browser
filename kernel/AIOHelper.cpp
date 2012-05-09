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

#include "AIOHelper.h"
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <errno.h>

#ifdef __APPLE__
#include <boost/bind.hpp>

static
void handler(const boost::system::error_code& error, // Result of operation.
              
             std::size_t bytes_transferred,           // Number of bytes written from the
             // buffers. If an error occurred,
             // this will be less than the sum
             // of the buffer sizes.
             void* buf,
             size_t count)
{
    if (bytes_transferred != count)
        std::cerr << error.message() << std::endl;
    free(buf);
}
#endif

AIOHelper* AIOHelper::_instance = NULL;

AIOHelper::AIOHelper()
{
}

AIOHelper* AIOHelper::instance()
{
    if (_instance == NULL) {
        _instance = new AIOHelper();
    }
    return _instance;
}

AIOHelper::~AIOHelper()
{
#ifdef __APPLE__
    std::map<int, posix::stream_descriptor*>::iterator iter;
    for (iter = m_fds.begin(); iter != m_fds.end(); ++iter) {
        delete iter->second;
    }
    m_fds.clear();
#endif
}

// to free allocated memory and finish unfinished job
void AIOHelper::gc()
{
#ifdef __APPLE__
    m_io_service.reset();
    m_io_service.poll();
#else    
    for (std::list<aiocb*>::iterator iter = m_cbs.begin();
         iter != m_cbs.end();) {
        aiocb* cb = *iter;
        if (aio_error(cb) != EINPROGRESS) {
            // clean up
            iter = m_cbs.erase(iter);
            int ret = aio_return(cb);
            if ((size_t)ret != cb->aio_nbytes) {
                std::cerr << "aio_write doesn't complete" << std::endl;
                // we currently do not handle this because this is so rare...
                // if this does happen, we need to modify the write() function
                // to add a queue...
            }
            free((void *)(cb->aio_buf));
            delete cb;
        } else {
            iter ++;
        }
    }
#endif
}

ssize_t AIOHelper::write(int fd, const void *buf, size_t count)
{
#ifdef __APPLE__
    if (m_fds.find(fd) == m_fds.end()) {
        m_fds[fd] = new posix::stream_descriptor(m_io_service, ::dup(fd));
    }
    posix::stream_descriptor* output = m_fds[fd];
    void* buffer = malloc(count);
    memcpy(buffer, buf, count);
    boost::asio::async_write(*output,
                             boost::asio::buffer(buffer, count),
                             boost::bind(handler,
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred,
                                         buffer,
                                         count)
                             );
    return count;
#else
    aiocb* cb = new aiocb;
    memset(cb, 0, sizeof(aiocb));
    void* buffer = malloc(count);
    memcpy(buffer, buf, count);
    cb->aio_fildes = fd;
    cb->aio_buf = buffer;
    cb->aio_nbytes = count;
    int ret = aio_write(cb);
    if (ret != 0) {
        free(buffer);
        delete cb;
        // if we cannot use aio, use sync io instead
        return ::write(fd, buf, count);
    }
    // assume we complete the whole write
    m_cbs.push_back(cb);
    return count;
#endif
}




