CC = clang++
CFLAGS = -std=c++23 -pedantic -Wall -Wextra -g
TARGET = main.out

.PHONY: clean run build

build: main.out 

main.out: main.cpp
	${CC} ${CFLAGS} -o $@ $<

clean:
	@rm -f ./*.o
	@rm -f ./*.out
	@rm -rf ./*.out.*
