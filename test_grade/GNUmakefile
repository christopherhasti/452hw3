prog=try
ldflags:=-lreadline -lhistory -lncurses

# Update objects to compile your own deq.c
objs:= $(patsubst %.c,%.o,$(wildcard *.c))

try: $(objs)
	gcc -o $@ $(objs) $(ldflags) 

test: $(prog)
	Test/run $<