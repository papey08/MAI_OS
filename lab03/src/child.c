#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>

#include "misc.h"

int main(void) {
    int buf_size = 2;
    char *input_buffer = malloc(buf_size);
    memset(input_buffer, 0, buf_size);

    if (!input_buffer) {
        return -1;
    }

    int mem_fd = shm_open(MEM, O_RDWR, 0);
    if (mem_fd == -1) {
        print_error("failed to open shared memory");
        exit(-1);
    }

    sem_t *empty = sem_open(SEM_EMPTY, 0);
    if (empty == SEM_FAILED) {
        print(STDERR_FILENO, "ERROR: failed to open semaphore\n");
        exit(-1);
    }

    sem_t *full = sem_open(SEM_FULL, 0);
    if (empty == SEM_FAILED) {
        print(STDERR_FILENO, "ERROR: failed to open semaphore\n");
        exit(-1);
    }

    char *buf =
        mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0);
    if (!buf) {
        print_error("failed to mmap");
        exit(-1);
    }

    while (1) {
        sem_wait(empty);
        int count = read_line(STDIN_FILENO, &input_buffer, &buf_size);
        if (count <= 0) {
            buf[0] = 0;
            sem_post(full);
            break;
        }
        int res = 0;
        char *ptr = input_buffer;
        while (*ptr) {
            int f = atoi(ptr);
            res += f;
            ptr = strchr(ptr, ' ');
            if (!ptr)
                break;
            ptr++;
        }
        int n = itoa(res, buf, 1);
        buf[n++] = '\n';
        buf[n++] = 0;
        sem_post(full);
    }
    munmap(buf, MEM_SIZE);
    free(input_buffer);
    return 0;
}
