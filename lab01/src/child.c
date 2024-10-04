#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "misc.h"

int main(void) {
    int buf_size = 2;
    char *input_buffer = malloc(buf_size);
    memset(input_buffer, 0, buf_size);
    char output_buffer[64];

    if (!input_buffer) {
        return -1;
    }
    while (1) {
        int count = read_line(STDIN_FILENO, &input_buffer, &buf_size);
        if (count <= 0)
            break;
        int res = 0;
        char *ptr = input_buffer;
        while (*ptr) {
            int f = atoi(ptr);
            res += f;
            ptr = strchr(ptr, ' ');
            if (!ptr)
                break;
            ptr++;
        }
        int n = itoa(res, output_buffer, 1);
        output_buffer[n++] = '\n';
        write(STDOUT_FILENO, output_buffer, n);
    }
    free(input_buffer);
    return 0;
}
