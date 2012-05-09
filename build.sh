#!/bin/bash
if [ "$1" = "clean" ]; then
    #ant clean
    cd storage; make clean; cd ..;
    #cd log_storage; make clean; cd ..;
    cd kernel; make clean; cd ..;
    cd op-ui; make clean; cd ..;
    cd webapp/op2; make clean; cd ../..;
    cd cookie; make clean; cd ..;
    cd network; make clean; cd ..;
else
    echo Compiling kernel... 
    cd kernel; make -j4|| exit 0; cd ..;

    #echo Compiling log_storage...
    #cd log_storage; make -j4|| exit 0; cd ..;

    echo Compiling storage...
    cd storage; qmake || exit 0; make -j4 || exit 0; cd ..;
    
	echo Compiling op-ui...
    cd op-ui; qmake || exit 0 ; make -j4|| exit 0; cd ..;
    
	echo Compiling op2...
    cd webapp/op2; qmake || exit 0; make -j4|| exit 0; cd ../..;
    
    echo Compiling network...
    cd network; qmake || exit 0; make -j4|| exit 0; cd ..;

    echo Compiling cookies...
    cd cookie; qmake || exit 0; make -j4|| exit 0; cd ..;
    
    #echo Compiling Java components...
    #ant compile
fi
