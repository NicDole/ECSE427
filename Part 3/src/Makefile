CC=gcc
CFLAGS=-DNDEBUG -Wall -D FRAME_STORE_SIZE=$(framesize) -D VAR_STORE_SIZE=$(varmemsize)

mysh: shell.c interpreter.c shellmemory.c pcb.c queue.c schedule_policy.c
	$(CC) $(CFLAGS) -c shell.c interpreter.c shellmemory.c pcb.c queue.c schedule_policy.c
	$(CC) $(CFLAGS) -o mysh shell.o interpreter.o shellmemory.o pcb.o queue.o schedule_policy.o -lpthread

clean:
	rm -f mysh *.o
