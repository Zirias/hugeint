CFLAGS:= -std=c11 -Wall -Wextra -pedantic
CFLAGS_RELEASE:= -O2 -g0 $(CFLAGS)
CFLAGS_DEBUG:= -O0 -g3 $(CFLAGS)

all: factorial.exe factorial-debug.exe divide.exe divide-debug.exe

factorial.o factorial-debug.o divide.o divide-debug.o: hugeint.h

%-debug.o: %.c
	gcc -c $(CFLAGS_DEBUG) -o$@ $<

%.o: %.c
	gcc -c $(CFLAGS_RELEASE) -o$@ $<

factorial-debug.exe: hugeint-debug.o factorial-debug.o
	gcc -o$@ $^ 

factorial.exe: hugeint.o factorial.o
	gcc -o$@ $^
	strip --strip-all $@

divide-debug.exe: hugeint-debug.o divide-debug.o
	gcc -o$@ $^

divide.exe: hugeint.o divide.o
	gcc -o$@ $^
	strip --strip-all $@

clean:
	rm -f *.o

.PHONY: all clean
