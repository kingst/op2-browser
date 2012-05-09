all: plugin 

CC = g++
CFLAGS = -ggdb -Wall -I../kernel
LIBS = -lboost_regex 

OBJS = main.o Message.o

-include $(OBJS:.o=.d)

Message.cpp:
	ln -s ../kernel/Message.cpp .

plugin: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean: 
	rm -f plugin *.o *~ core.* *.d
