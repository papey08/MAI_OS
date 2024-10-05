#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>

void handle_error(const char* msg) {
    perror(msg);
    exit(1);
}

int main() {
    int pipe1[2], pipe2[2];
    pid_t child1, child2;
    char *input = NULL;
    size_t len = 0;
    ssize_t nread;
    int r = 0;

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        handle_error("Pipe failed");
    }

    char *file1 = NULL, *file2 = NULL;
    size_t file_len = 0;

    printf("Введите имя файла для дочернего процесса 1: ");
    getline(&file1, &file_len, stdin);
    file1[strcspn(file1, "\n")] = 0;

    printf("Введите имя файла для дочернего процесса 2: ");
    getline(&file2, &file_len, stdin);
    file2[strcspn(file2, "\n")] = 0;

    if ((child1 = fork()) == 0) {
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);

        execlp("./child1", "./child1", file1, NULL);
        handle_error("execve for child1 failed");
    }

    if ((child2 = fork()) == 0) {
        close(pipe2[1]);
        close(pipe1[0]);
        close(pipe1[1]); 

        dup2(pipe2[0], STDIN_FILENO);
        close(pipe2[0]);

        execlp("./child2", "./child2", file2, NULL);
        handle_error("execve for child2 failed");
    }

    close(pipe1[0]);
    close(pipe2[0]);

    srand(time(NULL));

    while (1) {
        printf("Введите строку (или 'exit' для завершения): ");
        nread = getline(&input, &len, stdin);
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

    printf("Работа завершена.\n");

    free(input);
    free(file1);
    free(file2);

    return 0;
}
