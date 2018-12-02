all: mac.a mac.j rf
	@wd=`pwd`; \
	i=`grep '^MACDIR=' makes | sed -e 's/.*=[ 	]*//' -e 's/[ 	]*$$//'`; \
	if [ "X$$i" = "X" -o "X`eval echo $$i`" != "X$$wd" ]; \
		then echo Change MACDIR in makes to $$wd; \
	fi

rf: rf.o
	$(CC) -o $@ rf.o

mac.a: mac.0189
	perl mac.pl mac.0189 | perl mac2.pl mac.odd > mac.a

mac.j: mac.a
	grep _ExitToShell mac.a | awk '{printf("0x%s\n", $$1)}' > mac.j
