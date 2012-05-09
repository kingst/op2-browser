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
#ifndef OP_NETWORK_REPLY_H
#define OP_NETWORK_REPLY_H
#include <QNetworkReply>
#include <QBuffer>

namespace OPNET {

    class OpNetworkAccessManager;

    class OpNetworkReply : public QNetworkReply {
	Q_OBJECT

	enum State {
	    Idle,
	    Opening,
	    Working,
	    Finished,
	    Aborted
	};
	    
    public:
	OpNetworkReply(int urlId, OpNetworkAccessManager* manager);
	~OpNetworkReply();
	void setInfo(QNetworkAccessManager::Operation op,
		     const QNetworkRequest& req);

	void abort();
	qint64 bytesAvailable() const;
    protected:
	virtual qint64 readData(char* data, qint64 maxSize);	

    private:
	int m_reqId;
	OpNetworkAccessManager* m_manager;
	State m_state;
	QBuffer m_readBuffer;
	int m_readIdx;
	int m_writeIdx;
	


    public:
	void feed(const QByteArray& data);
	void feedMetaData(const QByteArray& metaData);
    };
}

#endif
