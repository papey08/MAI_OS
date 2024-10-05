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
        const char msg[] = "error: failed to open pipe\n";
		write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    char filename[BUF_SIZE];
    const char msg[] = "Enter filename: ";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    int n = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    
    if (n > 0 && n < sizeof(filename)) {
        filename[n - 1] = '\0';
    } else {
        filename[sizeof(filename) - 1] = '\0';
    }
    
    pid = fork();
    if (pid == -1) {
        const char msg[] = "error: failed to make fork\n";
		write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(pipefd[0]);

        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execlp("./child", "child", filename, (char*)NULL);
        const char msg[] = "error: failed to execlp\n";
		write(STDERR_FILENO, msg, sizeof(msg));
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
