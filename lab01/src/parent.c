#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

void handle_error(const char *message) {
    write(STDERR_FILENO, message, strlen(message));
    exit(EXIT_FAILURE);
}

int main() {
    int pipe1[2], pipe2[2];

    if (pipe(pipe1) == -1) {
        handle_error("Ошибка при создании pipe1\n");
    }

    if (pipe(pipe2) == -1) {
        handle_error("Ошибка при создании pipe2\n");
    }

    char filename[128];

    write(STDOUT_FILENO, "Введите имя файла: ", 34);
    fsync(STDOUT_FILENO);
    int n = read(STDIN_FILENO, filename, sizeof(filename));

    if (n <= 0) {
        handle_error("Ошибка при чтении имени файла\n");
    }
    filename[n - 1] = '\0';

    pid_t pid = fork();

    if (pid > 0) {
        close(pipe1[0]);
        close(pipe2[1]);

        char input[128];
        int number, signal;

        while (1) {
            write(STDOUT_FILENO, "Введите число: ", 28);
            fsync(STDOUT_FILENO);
            int n = read(STDIN_FILENO, input, sizeof(input));

            if (n <= 0) {
                handle_error("Ошибка при чтении числа\n");
            }

            input[n - 1] = '\0';
            
            char *endptr;
            number = strtol(input, &endptr, 10);

            if (endptr == input || *endptr != '\0') {
                handle_error("Ошибка: вводите только числа\n");
            }

            if (write(pipe1[1], &number, sizeof(number)) == -1) {
                handle_error("Ошибка при записи в pipe1\n");
            }

            if (read(pipe2[0], &signal, sizeof(signal)) == -1) {
                handle_error("Ошибка при чтении из pipe2\n");
            }

            if (signal == 1) { 
                write(STDOUT_FILENO, "Программа завершена.\n", 40);
                break;
            }
        }

        close(pipe1[1]);
        close(pipe2[0]);

        if (wait(NULL) == -1) {
            handle_error("Ошибка при ожидании дочернего процесса\n");
        }
    } else if (pid == 0) {
        close(pipe1[1]);
        close(pipe2[0]);

        if (dup2(pipe1[0], STDIN_FILENO) == -1) {
            handle_error("Ошибка при перенаправлении pipe1 в stdin\n");
        }

        if (dup2(pipe2[1], STDOUT_FILENO) == -1) {
            handle_error("Ошибка при перенаправлении pipe2 в stdout\n");
        }

        execl("./child", "./child", filename, NULL);

        handle_error("Ошибка при запуске дочернего процесса\n");
    } else {
        handle_error("Ошибка при вызове fork\n");
    }

    return 0;
}
