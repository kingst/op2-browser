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

Note: THESE BUILD INSTRUCTIONS ARE DATED AND WILL BE UPDATED SOON...

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

Here are the details of building OP2.

* First you need to be able to build QtWebKit. Some troubleshooting
  can be found at http://trac.webkit.org/wiki/QtWebKit. In OP2 source
  directory, you need to run ./build-webapp.sh and it will fetch a
  version of recent QtWebkit?, do some necessary patching for OP2 and
  then build it. If you encounter some errors during building, you can
  look at the aforementioned website for solution. We suggest using
  pre-compiled package of QT framework from QT Nokia
  http://qt.nokia.com/downloads instead of the package from apt-get or
  yum.

* Second, you can ./build.sh to build OP2 framework. The only special
  library you need is libboost. You can get it form boost official
  site or apt-get.

* Finally, after you successfully build OP2, you can use ./run-op to
  launch your browser. You can also use ./run-op-iframe-handled to
  test the iframe isolation feature in OP2, though it is still under
  development.

Remarks
-------

For sandboxing, you can use SELinux and write you own configuration
file. You can refer to our paper
http://www.cs.illinois.edu/homes/kingst/Research_files/grier08.pdf If
you want flash, you can just install native flash plugin. If you want
better security, you can use nspluginwrapper. The secure NPAPI plugin
support in OP2 is still under development and not included in the
source code.
