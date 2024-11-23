#define _XOPEN_SOURCE 900
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <semaphore.h>

int is_num(char c) {
    return (c >= '0') && (c <= '9');
}

int read_line_of_int(int* res) {
    char c = 0;
    int i = 0, k = 0, sign = 1, r;
    int flag = 1;
    int buf = 0;
    int sum = 0;
    while (c != '\n') {
        while(1) {
            r = read(STDIN_FILENO, &c, sizeof(char));
            if (r < 1) {
                return r;
            }
            if ((c == '\n') || (c == ' ')) {
                break;
            }
            if (flag && (c == '-')) {
                sign = -1;
            } else if (!is_num(c)) {
                return -1;
            } else {
                buf = buf * 10 + c - '0';
                i++;
                if (flag) {
                    k++;
                    flag = 0;
                }
            }
        }
        sum = sum + buf * sign;
        buf = 0;
        flag = 1;
        sign = 1;
    }
    if (k == 0) {
        return 0;
    }
    *res = sum;
    return 1;
}

int main() {
    int shm = shm_open("/memory", O_RDWR, 0666);
    if (shm == -1) {
        char* msg = "fail to open shared memory\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(-1);
    }

    sem_t* sem_empty = sem_open("/semaphore_empty", O_EXCL);
    if (sem_empty == SEM_FAILED) {
        char* msg = "fail to open semaphore\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(-1);
    }

    sem_t* sem_full = sem_open("/semaphore_full", O_EXCL);
    if (sem_full == SEM_FAILED) {
        char* msg = "fail to open semaphore\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(-1);
    }
    char* ptr = mmap(0, sizeof(int) * 2, PROT_READ|PROT_WRITE, MAP_SHARED, shm, 0);
    int res = 0;
    while (read_line_of_int(&res) > 0) {
        sem_wait(sem_empty);
        ptr[0] = res;
        sem_post(sem_full);
    }
    ptr[1] = 0;
    close(STDOUT_FILENO);
    return 0;
}