#include <unistd.h>
#include <string.h>

int main() {
    char input[1024];
    ssize_t bytes_read;

    while ((bytes_read = read(STDIN_FILENO, input, sizeof(input))) > 0) {
        input[bytes_read] = '\0';
        for (int i = 0; i < bytes_read; i++) {
            if (input[i] == ' ') {
                input[i] = '_';
            }
        }
        write(STDOUT_FILENO, input, bytes_read);
    }
    return 0;
}
