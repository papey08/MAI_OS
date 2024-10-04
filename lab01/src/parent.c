#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define BUF_SIZE 1024

int main() {
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    char filename[256];
    printf("Enter filename: ");
    scanf("%255s", filename);

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        
        close(pipefd[0]);

        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execlp("./child", "child", filename, (char*)NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {

        close(pipefd[1]);

        char buffer[BUF_SIZE];
        int n;
        while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            write(STDOUT_FILENO, buffer, n);
        }

        close(pipefd[0]);

        wait(NULL);
    }

    return 0;
}
