#ifndef __MISC_H__
#define __MISC_H__

#include <stddef.h>

int itoa(long n, char *res, int d);
char *strnchr(const char *buf, char c, size_t len);
int read_line(int fd, char **buf, int *buf_size);
int print(int fd, const char *s);

#endif
