#ifndef __MISC_H__
#define __MISC_H__

#include <stddef.h>
#define MEM "/lab03_memory"
#define MEM_SIZE 128
#define SEM_EMPTY "/lab03_semaphore_empty"
#define SEM_FULL "/lab03_semaphore_full"


int itoa(long n, char *res, int d);
char *strnchr(const char *buf, char c, size_t len);
int read_line(int fd, char **buf, int *buf_size);
int print(int fd, const char *s);
int print_error(const char *s);

#endif
