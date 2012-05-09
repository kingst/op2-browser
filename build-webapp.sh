#!/bin/bash
if [ ! -e /proc/cpuinfo ]; then
    NCPU=4
else
    NCPU=`grep -c 'model name' /proc/cpuinfo`
    NCPU=`echo "$NCPU 1 + p" | dc`
fi

if [ ! -e webapp/WebKit ]; then
        cd webapp;
        if [ ! -e  WebKit-r54749.tar.bz2 ]; then
            wget http://builds.nightly.webkit.org/files/trunk/src/WebKit-r54749.tar.bz2
        fi
        tar -xvjf WebKit-r54749.tar.bz2; mv WebKit-r54749 WebKit
        cd WebKit; cat ../webkit_patches/*r54749.diff | patch -p0
        cd ../..
fi

if [ "$1" = "debug" ]; then
    webapp/WebKit/WebKitTools/Scripts/build-webkit --qt --debug --makeargs="-j$NCPU"
elif [ "$1" = "clean" ]; then
        webapp/WebKit/WebKitTools/Scripts/build-webkit --qt --clean
else
    webapp/WebKit/WebKitTools/Scripts/build-webkit --qt --release --makeargs="-j$NCPU"
fi
