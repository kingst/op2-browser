op2-browser
===========

A new web browser that is designed to support web-based applications
securely, called the OP web browser. This browser is based off of
research from the University of Illinois and we have put up here at
Git hub.  Old versions were hosted at google code.

This research is sponsored by the National Science Foundation, the
Office of Naval Research, the Air Force Office of Scientific Research,
the Microsoft/Intel UPCRC, and the Internet Services Research Center
(ISRC) of Microsoft Research.

OP2 uses QtWebKit as its web engine. Building OP2 framework itself is
very easy. Most of problems come from building QtWebKit.

OP2 is a usable browser. Right now, you can use it for daily browser
without major problem. But please remember that it is still a research
browser and never in the stage of final release. We have not turned
off error trace. So you probably will see some error output in your
console.

Details
-------

You can grab a recent source code here at github.

Here are the details of building OP2. We have tested this on Ubuntu 12.04 (64 and 32 bit)

* First install some other necessary tools in case you have not: g++, libsqlite3-dev, qt-sdk
On Ubuntu:  
	sudo apt-get install g++ libsqlite3-dev	qt-sdk

* Second you need to have QtWebKit
Option 1: build QtWebKit. 
	In OP2 source directory, you need to run ./build-webapp.sh and it will fetch a version of recent QtWebkit?, 
	do some necessary patching for OP2 and then build it. If you encounter some errors during building, you can 
	look at http://trac.webkit.org/wiki/QtWebKit for solution. 
  
Option 2: using pre-compiled QtWebKit. 
	We recommend getting pre-compiled QT framework from QT Nokia instead of the package from apt-get or yum: 
	Download prebuilt binary at: http://get.qt.nokia.com/qtwebkit/QtWebKit-2.2.0.tar.gz
	Extract the content to webapp/WebKit

* Next you need to build and install libboost. 

To install libboost on Ubuntu: 
	sudo apt-get install libboost-dev

	Build and compile boost regex library (binary form)
		download boost from: http://www.boost.org/ 
		build the regex lib, inside boost directory:
			./bootstrap.sh
			./b2
		copy regex lib to /usr/lib:
			sudo cp <built_regex_lib_location>/libboost_regex.so.<version #> /usr/lib/libboost_regex-mt.so.<version #>
			sudo ln -s /usr/lib/libboost_regex-mt.so.<version #> /usr/lib/libboost_regex-mt.so			
			in my case: 
				<version #> is 1.49.0 
				<built_regex_lib_location> was /usr/local/boost_1_49_0/bin.v2/libs/regex/build/gcc-4.6/release/threading-multi/


			
* Next, build OP2 using ./build.sh 

* After you successfully build OP2, you can use ./run-op to
  launch your browser. You can also use ./run-op-iframe-handled to
  test the iframe isolation feature in OP2, though it is still under
  development.
	If OP2 complains about missing libboost_regex.so.<version #>, try
	sudo ln -s /usr/lib/libboost_regex-mt.so.<version #> /usr/lib/libboost_regex.so.<version #> 
	
Remarks
-------

For sandboxing, you can use SELinux and write you own configuration
file. You can refer to our paper
http://www.cs.illinois.edu/homes/kingst/Research_files/grier08.pdf If
you want flash, you can just install native flash plugin. If you want
better security, you can use nspluginwrapper. The secure NPAPI plugin
support in OP2 is still under development and not included in the
source code.
