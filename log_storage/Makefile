all: log_storage dump_log op_perf

UNAME := $(shell uname)

CC = g++
CFLAGS = -g -W -I../kernel
LIBS = -lsqlite3

ifeq ($(UNAME),Darwin)
CFLAGS +=  -I /opt/local/include
LIBS +=  /opt/local/lib/libboost_regex-mt.a
else
LIBS += -lboost_regex-mt
endif

OBJS = main.o Message.o Cookie.o 
LOBJS = main_log.o Message.o LogStorage.o
DOBJS = main_dump.o Message.o DumpStorage.o
POBJS = main_perf.o Message.o DumpStorage.o

-include $(OBJS:.o=.d)

op_perf: $(POBJS)
	$(CC) -o $@ $(CFLAGS) $(POBJS) $(LIBS)

dump_log: $(DOBJS)
	$(CC) -o $@ $(CFLAGS) $(DOBJS) $(LIBS)

log_storage: $(LOBJS)
	$(CC) -o $@ $(CFLAGS) $(LOBJS) $(LIBS)

storage: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LIBS)

Message.cpp:
	ln -s ../kernel/Message.cpp .

%.d: %.cpp
	set -e; $(CC) -MM $(CFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean: 
	rm -f *.o *~ core.* *.d log_storage Message.cpp dump_log

test: testing/test.cpp Cookie.cpp
	g++ -g -W -c Cookie.cpp -o Cookie.o
	g++ -g -W -c testing/test.cpp -o testing/test.o
	g++ -o testing/test -g -W testing/test.o Cookie.o Message.o
