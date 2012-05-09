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

#include <qapplication.h>
#include <qpushbutton.h>

#include "Message.h"

class Counter : public QObject {
    Q_OBJECT

    private:

    public: 
          Counter() { };
          ~Counter() { };

    public slots:
        void doClickStuff(){ 

        }

    signals:
};

void* doPluginStuff(void* args) {
    int maxFd;
    fd_set readSet;
    int ret;
    int state = 0;

    fprintf(stderr, "sample plugin going to send message for cookie\n");

    // we are going to access a cookie here, send a message to the cookie store 
    // and make sure that no matter what cookie we try and access (different domains)
    // our access controls check it properly.
    string domain("www.google.com");

    Message msg(PLUGIN_ID, COOKIE_ID, 0, MSG_COOKIE_GET, 0, domain);
    msg.writeMessage(STDOUT_FILENO);

    fprintf(stderr, "sample plugin sent message\n");

    state = 1;

    while(1) {
        FD_ZERO(&readSet);
        FD_SET(STDIN_FILENO, &readSet);

        ret = select(STDIN_FILENO+1, &readSet, NULL, NULL, NULL);

        if(FD_ISSET(0, &readSet)) {
            Message m;
            m.readMessage(STDIN_FILENO);

            if(state >= 1 && m.getMsgType() == MSG_COOKIE_GET) {

                fprintf(stderr, "got a cookie! contents: \"%s\"\n", m.getStringData().c_str());
                state++;
            }
        }
    }
};

int main(int argc, char **argv)
{ 
    QApplication a(argc, argv);

    Counter tt;

    QPushButton hello("plugin", 0);
    hello.resize(100,30);

    a.setMainWidget(&hello);
    hello.show();
    a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()) );
    a.connect(&hello, SIGNAL(clicked()), &tt, SLOT(doClickStuff()) );

    pthread_t tmp;
    pthread_create(&tmp, NULL, doPluginStuff, NULL); 

    return a.exec();

}

#include "main.moc"
