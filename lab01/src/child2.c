#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

void HandleError(const char* msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    write(STDERR_FILENO, "\n", 1);
    exit(1);
}

void Print(const char* msg) {
    write(STDOUT_FILENO, msg, strlen(msg));
}

void RemoveVowels(char* str) {
    char* p = str;
    char* q = str;
    while (*p) {
        if (!strchr("AEIOUaeiou", *p)) {
            *q++ = *p;
        }
        p++;
    }
    *q = '\0';
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        HandleError("Usage: <program> <output_file>");
    }

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        HandleError("Cannot open file");
    }

    char buffer[1024];
    ssize_t bytesRead;

    while ((bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        RemoveVowels(buffer);
        write(fd, buffer, strlen(buffer));
        write(fd, "\n", 1);
    }

    close(fd);

    return 0;
}
