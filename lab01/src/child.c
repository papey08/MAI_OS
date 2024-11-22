#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

void print_error(int pipe_fd, const char *msg) {
    write(pipe_fd, msg, strlen(msg));
}

int ends_with_dot_or_semicolon(const char *str) {
    int len = strlen(str);
    return (len > 0 && (str[len - 1] == '.' || str[len - 1] == ';'));
}

int main(int argc, char* argv[]) {
    int pipe1[2] = {3, 4};
    int pipe2[2] = {5, 6};
    char buffer[BUFFER_SIZE];
    char* error_msg;

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        error_msg = "Ошибка открытия файла\n";
        print_error(pipe2[1], error_msg);
        close(fd);
        close(pipe1[0]);
        close(pipe2[1]);
        return -1;
    }

    while (1) {
        int len = read(pipe1[0], buffer, BUFFER_SIZE);
        if (len <= 0) break;
        buffer[len - 1] = '\0';
        if (ends_with_dot_or_semicolon(buffer)) {
            write(fd, buffer, strlen(buffer));
            write(fd, "\n", 1);
            print_error(pipe2[1], "SUCCESS");
        } else {
            error_msg = "Строка не соответствует правилу!\n";
            print_error(pipe2[1], error_msg);
        }
    }

    close(fd);
    close(pipe1[0]);
    close(pipe2[1]);
    return 0;
}




