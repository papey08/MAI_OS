CC = gcc
CFLAGS = -Wall -Wextra -fPIC
LDFLAGS = -shared

all: first_fit.so mckusick_karels.so main

first_fit.so: first_fit_allocator.c allocator.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

mckusick_karels.so: mckusick_karels_allocator.c allocator.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

main: main.c
	$(CC) $(CFLAGS) -o $@ $< -ldl

clean:
	rm -f *.so main
