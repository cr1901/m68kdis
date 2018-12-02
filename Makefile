CFLAGS=-DDEBUG -g -D_ANSI_C_SOURCE
LIBS=-lm
SRCS=dis.c  inst1.c   inst2.c   main.c   utils.c   afline.c   pgen.c   fgen.c
OBJS=$(SRCS:.c=.o)
PROG=m68kdis

$(PROG): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

$(OBJS): dis.h
inst2.o fgen.o pgen.o: addr.h

hex: hex.o
	$(CC) -o $@ hex.o

clean:
	-rm -f $(PROG) $(OBJS) hex hex.o
