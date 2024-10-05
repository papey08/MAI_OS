#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int is_num(char c) {
    return (c >= '0') && (c <= '9');
}

int read_line_of_int(int* res) {
    char c = 0;
    int i = 0, k = 0, sign = 1, r;
    int flag = 1;
    int buf = 0;
    int sum = 0;
    while (c != '\n') {
        while(1) {
            r = read(STDIN_FILENO, &c, sizeof(char));
            if (r < 1) {
                return r;
            }
            if ((c == '\n') || (c == ' ')) {
                break;
            }
            if (flag && (c == '-')) {
                sign = -1;
            } else if (!is_num(c)) {
                return -1;
            } else {
                buf = buf * 10 + c - '0';
                i++;
                if (flag) {
                    k++;
                    flag = 0;
                }
            }
        }
        sum = sum + buf * sign;
        buf = 0;
        flag = 1;
        sign = 1;
    }
    if (k == 0) {
        return 0;
    }
    *res = sum;
    return 1;
}

int main() {
    int res = 0;
    while (read_line_of_int(&res) > 0) {
        write(STDOUT_FILENO, &res, sizeof(int));
    }
    close(STDOUT_FILENO);
    return 0;
}