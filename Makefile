all: factorial.exe factorial-debug.exe

factorial-debug.exe: factorial.c
	gcc -O0 -g3 -std=c11 -Wall -Wextra -pedantic -o$@ $<

factorial.exe: factorial.c
	gcc -O3 -g0 -std=c11 -Wall -Wextra -pedantic -o$@ $<
	strip --strip-all $@


