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
#include "Cookie.h"

#include <iostream>
#include <algorithm>

using namespace std;

void Cookie::fillCookie(string fullcookie) {

    unsigned long loc = fullcookie.find(";");
    if(loc == string::npos) {
        cerr << "problem parsing cookie!\n";
        return;
    }

    domain = fullcookie.substr(0, loc);

    params = parseParams(fullcookie.substr(loc+1, fullcookie.length()));
}

void Cookie::fillCookie(char * d, char* f, char* p, char* s, char* t, char* n, char* v, char* c) {
    if(d == NULL) d = "";
    if(f == NULL) f = "";
    if(p == NULL) p = "";
    if(s == NULL) s = "";
    if(t == NULL) t = "";
    if(n == NULL) n = "";
    if(v == NULL) v = "";
    if(c == NULL) c = "";
    
    domain = d;

    params.push_back(string(f));
    params.push_back(string(p));
    params.push_back(string(s));
    params.push_back(string(t));
    params.push_back(string(n));
    params.push_back(string(v));
    params.push_back(string(c));
}

vector<string> Cookie::parseParams(string toparse) {

    vector<string> parsedParams;
    unsigned long loc = 0;
    int count = 0;

    while((loc = toparse.find(";")) != string::npos) {
        
        string temp = toparse.substr(0, loc);
        parsedParams.push_back(temp);
        toparse = toparse.substr(loc+1, toparse.length());
        count++;
    } 

    if(!toparse.empty()) {
        parsedParams.push_back(toparse);
    }

    return parsedParams;
}

template<class T> struct assemble : public unary_function<T, void>
{
    assemble() : result("") {}
    void operator() (T x) { result += x + ';'; }
    string result;
};

string Cookie::toString() {

    assemble<string> P = for_each(params.begin(), params.end(), assemble<string>());

    return P.result;

}
