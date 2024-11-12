#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

void reverse_string(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = temp;
    }
}

int main() {
    char buffer[BUFFER_SIZE];

    while (1) {
        ssize_t bytes_read = read(0, buffer, BUFFER_SIZE); 
        if (bytes_read <= 0) {
            exit(EXIT_SUCCESS);
        }
        buffer[bytes_read - 1] = '\0'; 

        if (strlen(buffer) == 0) {
            break;
        }

        reverse_string(buffer);
        write(1, buffer, strlen(buffer));
        write(1, "\n", 1);
    }

    return 0;
}
