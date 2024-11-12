#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int itoa(long n, char *res, int d) {
    int neg = 0;
    if (n < 0) {
        neg++;
        n *= -1;
        res[0] = '-';
        res++;
    }
    int i = 0;
    while (n > 0) {
        res[i++] = '0' + (n % 10);
        n /= 10;
    }
    while (i < d) {
        res[i++] = '0';
    }
    for (int j = 0; j < i / 2; j++) {
        char tmp = res[j];
        res[j] = res[i - j - 1];
        res[i - j - 1] = tmp;
    }
    res[i] = 0;
    return i + neg;
}

char *strnchr(const char *buf, char c, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == c)
            return (char *)(buf + i);
        if (buf[i] == 0)
            return 0;
    }
    return 0;
}

int read_line(int fd, char **buf, int *buf_size) {
    int count = strnchr(*buf, 0, *buf_size) - *buf + 1;
    for (int i = 0; i < *buf_size - count; i++) {
        (*buf)[i] = (*buf)[i + count];
    }
    count = strnchr(*buf, 0, *buf_size) - *buf;
    int read_cur;
    do {
        if (count + 1 >= *buf_size) {
            int new_size = *buf_size * 2;
            char *tmp = realloc(*buf, new_size);
            if (!tmp)
                return count;
            *buf = tmp;
            *buf_size = new_size;
        }
        read_cur = read(fd, *buf + count, *buf_size - count - 1);
        count += read_cur;
        (*buf)[count] = 0;
    } while (read_cur > 0 && !strnchr(*buf, '\n', *buf_size));
    int line_len = strnchr(*buf, '\n', *buf_size) - *buf;
    if (line_len < 0)
        return count;
    (*buf)[line_len] = 0;
    return line_len + 1;
}

int print(int fd, const char *s) {
    int n = strlen(s);
    return write(fd, s, n);
}
