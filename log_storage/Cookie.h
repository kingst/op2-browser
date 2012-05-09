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
#ifndef __COOKIE_H__
#define __COOKIE_H__

#include <string>
#include <vector>
#include <iostream>

using namespace std;

class Cookie {

    //
    //.netscape.com TRUE / FALSE 946684799 NETSCAPE_ID 100103 
    //domain, flag, path, secure, expiration, name, value
    //
    private:
        string domain;
        vector<string> params;

        void emptyParams() { params.clear(); };
        vector<string> parseParams(string toparse);

    public:
        /*
        Cookie(string d, bool f, string p, bool s, u32 ts, string n, string v): 
            domain(d), flag(f), path(p) secure(s), timestamp(ts), name(n), value(v) 
        {


        };
        */

        Cookie() { };
        Cookie(string d) : domain(d) {};

    Cookie(char * d, char* f, char* p, char* s, char* t, char* n, char* v, char* c): 
            domain(d) 
        {
            emptyParams();
            params.push_back(string(f));
            params.push_back(string(p));
            params.push_back(string(s));
            params.push_back(string(t));
            params.push_back(string(n));
            params.push_back(string(v));
            params.push_back(string(c));
        };

        void fillCookie(string fullcookie);
    void fillCookie(char * d, char* f, char* p, char* s, char* t, char* n, char* v, char* c); 
        void createBlank() { 
            emptyParams();
            params.push_back(string(" ")); params.push_back(string(" "));
            params.push_back(string(" ")); params.push_back(string(" "));
            params.push_back(string(" ")); params.push_back(string(" "));
            params.push_back(string(" ")); };

        string toString();
        string getDomain() { return domain; };

        string operator[](int x) { return params[x]; };
        int size() { return params.size(); };
};

#endif
