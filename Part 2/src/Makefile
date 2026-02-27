CC = gcc
CFLAGS = -pthread
FMT = indent

mysh: shell.c interpreter.c shellmemory.c scheduler.c
	$(CC) $(CFLAGS) -o mysh shell.c interpreter.c shellmemory.c scheduler.c

style: shell.c shell.h interpreter.c interpreter.h shellmemory.c shellmemory.h
	$(FMT) $?

clean:
	$(RM) mysh *.o *~

