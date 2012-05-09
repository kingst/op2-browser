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
#include "DomainName.h"

#include <boost/regex.hpp>
#include <algorithm>

using namespace boost;


void DomainName::print()  {
    cerr << "**** DOMAIN LABEL ****\n";
    cerr << "Protocol: <" << protocol << ">\n";
    cerr << "TLD: <" << tld << ">\n";
    cerr << "SLD: <" << sld << ">\n";
    cerr << "other: ";
    for(vector<string>::iterator iter = levels.begin(); iter != levels.end(); iter++)
        cerr << "<" << *iter << "> ";
    cerr << endl;
    cerr << "**** END ****\n";
};

string findProtocol(string &a) {
    boost::regex prot("^(http://|https://)(.*)");
    boost::cmatch what;

    string protocol;

    // first match the protocol
    if(boost::regex_match(a.c_str(), what, prot)) {
        protocol.assign(what[1].first, what[1].second);
        a.assign(what[2].first, what[2].second);
    }
    return protocol;
};

string findPath(string &a) {
    boost::regex prot("^([\\w\\d\\-\\.]*)/(.*)");
    boost::cmatch what;

    string path;

    // first match the path
    if(boost::regex_match(a.c_str(), what, prot)) {
        path.assign(what[2].first, what[2].second);
        a.assign(what[1].first, what[1].second);
    }
        return path;
};

string findTLD(string &a) {
    boost::regex prot("(.*)\\.(.{2,4})");
    boost::cmatch what;

    string tld;

    // first match the tld
    if(boost::regex_match(a.c_str(), what, prot)) {
        tld.assign(what[2].first, what[2].second);
        a.assign(what[1].first, what[1].second);
    }
    return tld;
};

string findDomain(string &a) {
    boost::regex prot("(.*)\\.(.*)");
    boost::cmatch what;

    string tld;

    // first match the tld
    if(boost::regex_match(a.c_str(), what, prot)) {
        tld.assign(what[2].first, what[2].second);
        a.assign(what[1].first, what[1].second);
    }
    return tld;
}

bool DomainName::sameDomainStrict(DomainName a, DomainName b) { 
    return (a == b);
}

void DomainName::fromString(string a) {
    std::transform(a.begin(), a.end(), a.begin(), ::tolower);
    url = a;
    protocol = findProtocol(a);
    path = findPath(a);
    tld = findTLD(a);
    sld = findDomain(a);

    string tmp = a;
    while(!tmp.empty()) {
        tmp = findDomain(a);
        if(tmp.empty() && !a.empty()) 
            levels.insert(levels.begin(), a.substr(0, a.length()));
        else 
            levels.insert(levels.begin(), tmp);
    }

}

bool DomainName::sameDomainStrict(string a, string b) {
    DomainName aparsed, bparsed;

    aparsed.fromString(a);
    bparsed.fromString(b);

    return (aparsed == bparsed);
}

bool DomainName::sameDomainDomainOnly(DomainName a, DomainName b) {
    if(a.tld == b.tld && 
            a.sld == b.sld &&
            a.levels.size() == b.levels.size() &&
            a.levels == b.levels) {
        return true;
    }
    return false;
}

bool DomainName::sameOrigin(DomainName a, DomainName b)
{
    if (a.port = b.port &&
	a == b) {
	return true;
    }
    return false;
}

DomainName::DomainName()
    : port(80)
{
}
