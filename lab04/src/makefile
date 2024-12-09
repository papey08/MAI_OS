CC = clang
CFLAGS = -std=gnu18 -pedantic -Wall -Wextra -g
TARGET = main.out

.PHONY: clean run build

build: main.out

main.out: main.c list.c
	${CC} ${CFLAGS} -o $@ main.c

clean:
	@rm -f ./*.o
	@rm -f ./*.out
	@rm -rf ./*.out.*
