CC = clang
CFLAGS = -std=gnu17 -pedantic -Wall -Wextra -g
TARGET = main.out

.PHONY: clean run build

build: main.out 

main.out: main.o 
	${CC} -o $@ $^

main.o: main.c
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	@rm -f ./*.o
	@rm -f ./*.out
	@rm -rf ./*.out.*
