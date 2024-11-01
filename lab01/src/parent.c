#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>

void HandleError(const char* msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    write(STDERR_FILENO, "\n", 1);
    exit(1);
}

ssize_t Getline(char **lineptr, size_t *n, int fd) {
    if (*lineptr == NULL) {
        *lineptr = malloc(128);
        *n = 128;
    }

    size_t pos = 0;
    char c;
    while (read(fd, &c, 1) == 1) {
        if (pos >= *n - 1) {
            *n *= 2;
            *lineptr = realloc(*lineptr, *n);
        }
        (*lineptr)[pos++] = c;
        if (c == '\n') {
            break;
        }
    }

    if (pos == 0) {
        return -1;
    }

    (*lineptr)[pos] = '\0';
    return pos;
}

void Print(const char* msg) {
    write(STDOUT_FILENO, msg, strlen(msg));
}

int main() {
    int pipe1[2], pipe2[2];
    pid_t child1, child2;
    char *input = NULL;
    size_t len = 0;
    ssize_t nread;
    int r = 0;

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        HandleError("Pipe failed");
    }

    char *file1 = NULL, *file2 = NULL;
    size_t file_len = 0;

    Print("Введите имя файла для дочернего процесса 1: ");
    Getline(&file1, &file_len, STDIN_FILENO);
    file1[strcspn(file1, "\n")] = 0;

    Print("Введите имя файла для дочернего процесса 2: ");
    Getline(&file2, &file_len, STDIN_FILENO);
    file2[strcspn(file2, "\n")] = 0;

    if ((child1 = fork()) == 0) {
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);

        execlp("./child1", "./child1", file1, NULL);
        HandleError("execve for child1 failed");
    }

    if ((child2 = fork()) == 0) {
        close(pipe2[1]);
        close(pipe1[0]);
        close(pipe1[1]);

        dup2(pipe2[0], STDIN_FILENO);
        close(pipe2[0]);

        execlp("./child2", "./child2", file2, NULL);
        HandleError("execve for child2 failed");
    }

    close(pipe1[0]);
    close(pipe2[0]);

    srand(time(NULL));

    while (1) {
        Print("Введите строку (или 'exit' для завершения): ");
        nread = Getline(&input, &len, STDIN_FILENO);
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "exit") == 0) {
            break;
        }

        r = rand() % 5 + 1;

        if (r == 3) {
            write(pipe2[1], input, nread);
        } else {
            write(pipe1[1], input, nread);
        }
    }

    close(pipe1[1]);
    close(pipe2[1]);

    wait(NULL);
    wait(NULL);

    Print("Работа завершена.\n");

    free(input);
    free(file1);
    free(file2);

    return 0;
}
