CC = clang
CFLAGS = -std=gnu17 -pedantic -Wall -Wextra -g
TARGET = main.out

.PHONY: clean run build

build: main.out child.out

main.out: main.o misc.o
	${CC} -o $@ $^

child.out: child.o misc.o
	${CC} -o $@ $^

main.o: main.c misc.h
	${CC} ${CFLAGS} -c -o $@ $<

child.o: child.c misc.h
	${CC} ${CFLAGS} -c -o $@ $<

misc.o: misc.c misc.h
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	@rm -f ./*.o
	@rm -f ./*.out
	@rm -rf ./*.out.*
