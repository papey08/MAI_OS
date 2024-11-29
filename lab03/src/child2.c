#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>

#define SHM_SIZE 1024

void handle_error(const char *msg) {
    write(2, msg, strlen(msg));
    _exit(EXIT_FAILURE);
}

int main() {
    // Open shared memory using shm_open
    int fd = shm_open("/shared_memory", O_RDWR, 0666);
    if (fd == -1) handle_error("shm_open");

    // Map shared memory into process's address space
    char *shared_memory = (char *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_memory == MAP_FAILED) handle_error("mmap");

    // Open semaphores
    sem_t *sem_parent = sem_open("/sem_parent", 0);
    sem_t *sem_child2 = sem_open("/sem_child2", 0);

    if (sem_parent == SEM_FAILED || sem_child2 == SEM_FAILED)
        handle_error("sem_open");

    while (1) {
        sem_wait(sem_child2);

        if (strcmp(shared_memory, "") == 0) break;

        for (int i = 0; shared_memory[i] != '\0'; i++) {
            if (shared_memory[i] == ' ')
                shared_memory[i] = '_';
        }

        sem_post(sem_parent);
    }

    if (sem_close(sem_parent) == -1) handle_error("sem_close sem_parent");
    if (sem_close(sem_child2) == -1) handle_error("sem_close sem_child2");

    if (munmap(shared_memory, SHM_SIZE) == -1) handle_error("munmap");

    return 0;
}
