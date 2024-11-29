# CFLAGS = -Wall -Wextra -pedantic -g -fdiagnostics-color=always -O2

all: parent child1 child2

parent: parent.c
	gcc -o final parent.c

child1: child1.c
	gcc -o child1 child1.c;

child2: child2.c
	gcc -o child2 child2.c;

clean:
	rm child1 child2 final shared_memory