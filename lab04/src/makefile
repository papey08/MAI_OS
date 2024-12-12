CC = clang
CFLAGS = -std=gnu18 -pedantic -Wall -Wextra -g
TARGET = main.out list.so buddy.so

.PHONY: clean run build

build: ${TARGET}

main.out: main.c
	${CC} ${CFLAGS} -o $@ $<

list.so: list_allocator.c
	${CC} ${CFLAGS} -shared -o $@ $<

buddy.so: buddy_allocator.c
	${CC} ${CFLAGS} -shared -o $@ $<


clean:
	@rm -f ${TARGET}
	@rm -rf ./*.dSYM
