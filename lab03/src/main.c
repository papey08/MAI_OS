#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#include "misc.h"

int main(int argc, char *argw[]) {
    char *prog_name;
    if (argc >= 2)
        prog_name = argw[1];
    else
        prog_name = "./child.out";

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
        print_error("failed to open file");
        exit(-1);
    }
    free(fname);

    int mem_fd = shm_open(MEM, O_RDWR | O_CREAT, 0777);
    if (mem_fd == -1) {
        print_error("failed to open shared memory");
        close(fd);
        exit(-1);
    }
    if (ftruncate(mem_fd, MEM_SIZE) == -1) {
        print_error("failed to truncate shared memory");
        close(fd);
        shm_unlink(MEM);
        exit(-1);
    }

    sem_t *empty = sem_open(SEM_EMPTY, O_RDWR | O_CREAT, 0777, 1);
    if (empty == SEM_FAILED) {
        print_error("failed to create empty semaphore");
        close(fd);
        shm_unlink(MEM);
        exit(-1);
    }

    sem_t *full = sem_open(SEM_FULL, O_RDWR | O_CREAT, 0777, 0);
    if (full == SEM_FAILED) {
        print_error("failed to create full semaphore");
        close(fd);
        shm_unlink(MEM);
        sem_unlink(SEM_EMPTY);

        exit(-1);
    }

    pid_t pid = fork();
    if (pid == 0) {
        dup2(fd, STDIN_FILENO);
        close(fd);
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
        print_error("failed to fork process");
        exit(-1);
    } else {
        char *buf =
            mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0);
        if (!buf) {
            print_error("failed to mmap");
            sem_unlink(SEM_EMPTY);
            sem_unlink(SEM_FULL);
            shm_unlink(MEM);
            exit(-1);
        }
        while (1) {
            sem_wait(full);
            if (*buf == 0)
                break;
            print(STDOUT_FILENO, buf);
            sem_post(empty);
        }
        waitpid(pid, 0, 0);
    }
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);
    shm_unlink(MEM);

    return 0;
}
