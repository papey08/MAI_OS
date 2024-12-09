CC = clang
CFLAGS = -std=gnu18 -pedantic -Wall -Wextra -g
TARGET = main.out link.so

.PHONY: clean run build

build: ${TARGET}

main.out: main.c
	${CC} ${CFLAGS} -o $@ $<

link.so: list_allocator.c
	${CC} ${CFLAGS} -shared -o $@ $<

clean:
	@rm -f ${TARGET}
	@rm -rf ./*.dSYM
