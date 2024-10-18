#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>

#define BUFSIZE 1024

int main() {

    const char start_msg[] = "Type name of your file: ";
    write(STDOUT_FILENO, start_msg, sizeof(start_msg));

    char filename[BUFSIZE];
    int n = read(STDIN_FILENO, filename, sizeof(filename) - 1);

    if (n > 0 && n < sizeof(filename)) {
        filename[n - 1] = '\0';
    } else {
        filename[sizeof(filename) - 1] = '\0';
    }

    int fd[2]; 
    if (pipe(fd) == -1) {
        const char msg[] = "ERROR: pipe does not open\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    switch (pid)
    {
    case -1:
        const char msg[] = "ERROR: new process has not been created\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
        break;
    case 0:
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

        execlp("./child", "./child", filename, (char*)NULL);

        const char exec_msg[] = "ERROR: process has not started\n";
        write(STDERR_FILENO, exec_msg, sizeof(exec_msg));
        exit(EXIT_FAILURE);
        break;
    default:
        close(fd[1]);
        char print_msg[BUFSIZE];
        int n = read(fd[0], print_msg, sizeof(print_msg));
        write(STDOUT_FILENO, print_msg, n);
        close(fd[0]);
        break;
    }
}