#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "misc.h"

int main(int argc, char *argw[]) {
    char *prog_name;
    if (argc >= 2)
        prog_name = argw[1];
    else
        prog_name = "./child.out";

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        print(STDERR_FILENO, "ERROR: failed to create pipe\n");
        exit(-1);
    }

    int buf_size = 64;
    char *fname = malloc(buf_size);
    *fname = 0;
    if (read_line(STDIN_FILENO, &fname, &buf_size) <= 0) {
        print(STDERR_FILENO,
              "ERROR: failed read filename from standart input\n");
        exit(-1);
    }
    int fd = open(fname, O_RDONLY);
    if (fd == -1) {
        char out[1024] = {0};
        strcat(out, "ERROR: failed to open file: \"");
        strcat(out, fname);
        strcat(out, "\"\n");
        strcat(out, strerror(errno));
        strcat(out, "\n");
        print(STDERR_FILENO, out);
        exit(-1);
    }
    free(fname);

    pid_t pid = fork();
    if (pid == 0) {
        dup2(fd, STDIN_FILENO);
        close(fd); // close fd, it was duplicated by dup2

        close(pipefd[0]); // close read. we only need to write to pipe
        dup2(pipefd[1], STDOUT_FILENO); // redirect output into pipe
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]); // close pipe, it was duplicated

        char *argv[] = {prog_name, "", NULL};
        if (execv(prog_name, argv) == -1) {
            char out[1024] = {0};
            strcat(out, "ERROR: failed to launch process \"");
            strcat(out, prog_name);
            strcat(out, "\"\n");
            strcat(out, strerror(errno));
            strcat(out, "\n");
            print(STDERR_FILENO, out);
            exit(-1);
        }
    } else if (pid == -1) {
        char out[1024] = {0};
        strcat(out, "ERROR: failed to fork process\n");
        strcat(out, strerror(errno));
        strcat(out, "\n");
        print(STDERR_FILENO, out);
        exit(-1);
    } else {
        char buffer[128];
        close(pipefd[1]); // close write we only need to read from the pipe
        while (1) {
            int n = read(pipefd[0], buffer, sizeof(buffer));
            if (n == 0)
                break;
            write(STDOUT_FILENO, buffer, n);
        }
        waitpid(pid, 0, 0);
    }
    return 0;
}
