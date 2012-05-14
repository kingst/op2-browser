all: kernel msgdump

UNAME := $(shell uname)

CC = g++
CFLAGS = -ggdb -W -D__OPKERNEL__

ifeq ($(UNAME),Darwin)
CFLAGS += -I /opt/local/include
LDFLAGS = /opt/local/lib/libboost_regex-mt.a /opt/local/lib/libboost_system-mt.a
else
LDFLAGS = -lboost_regex-mt -lrt 
endif

CFLAGS += -I ../webapp/WebKit/WebKitLibraries/WebCoreSQLite3

MOBJS = msgdump.o
OBJS = main.o Message.o WebAppMgr.o Access.o DomainName.o WebAppCache.o AIOHelper.o

-include $(OBJS:.o=.d)

msgdump: $(MOBJS)
	$(CC) -o $@ $(CFLAGS) $(MOBJS) -lsqlite3

kernel: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LDFLAGS)

%.d: %.cpp
	set -e; $(CC) -MM $(CFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean: 
	rm -f kernel msgdump *.o *~ core.* *.d