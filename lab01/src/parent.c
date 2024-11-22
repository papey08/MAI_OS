#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 1024

void print_error(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}

int main() {
    int pipe1[2] = {3, 4};
    int pipe2[2] = {5, 6};
    pid_t pid;
    char filename[BUFFER_SIZE];
    char input[BUFFER_SIZE];
    char error_msg[BUFFER_SIZE];

    const char start_msg[] = "Введите имя файла: ";
    write(STDOUT_FILENO, start_msg, sizeof(start_msg));
    input[strcspn(filename, "\n")] = 0;
    int filename_len = read(STDIN_FILENO, filename, sizeof(filename));
    if (filename_len <= 1) {
        print_error("Ошибка ввода имени файла\n");
        exit(EXIT_FAILURE);
    }
    
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        print_error("Ошибка при создании pipe\n");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        print_error("Ошибка при создании процесса\n");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        close(pipe1[1]);
        close(pipe2[0]);
        execlp("./child", "./child", filename, (char *) NULL);
        print_error("Ошибка при запуске дочернего процесса\n");
        exit(EXIT_FAILURE);
    }

    close(pipe1[0]);
    close(pipe2[1]);

    const char msg1[] = "Введите строку (или 'exit' для выхода):\n";
    write(STDOUT_FILENO, msg1, sizeof(msg1));

    while (1) {
        int input_len = read(STDIN_FILENO, input, sizeof(input));
        input[strcspn(input, "\n")] = 0;
        if (strcmp(input, "exit") == 0) {
            break;
        }
        if (input_len <= 1) {
            print_error("Ошибка ввода строки\n");
            break;
        }

        if (write(pipe1[1], input, input_len) == -1) {
            print_error("Ошибка записи в pipe1\n");
            break;
        }

        int error_len = read(pipe2[0], error_msg, BUFFER_SIZE);
        error_msg[error_len] = '\0';
        if (error_len > 0) {
            if (strcmp(error_msg, "SUCCESS") != 0) {
                write(STDERR_FILENO, error_msg, error_len);
            }
        }
    }
    close(pipe1[1]);
    close(pipe2[0]);
    wait(NULL);
    return 0;
}








