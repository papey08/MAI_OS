#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <string.h>

#define SHM_SIZE 1024

void handle_error(const char *msg) {
    write(2, msg, strlen(msg));
    _exit(EXIT_FAILURE);
}

int main() {
    key_t key = ftok("shared_memory", 65);
    if (key == -1) handle_error("ftok");

    int shmid = shmget(key, SHM_SIZE, 0666);
    if (shmid == -1) handle_error("shmget");

    char *shared_memory = (char *)shmat(shmid, NULL, 0);
    if (shared_memory == (char *)-1) handle_error("shmat");

    for (int i = 0; shared_memory[i] != '\0'; i++) {
        if (shared_memory[i] == ' ')
            shared_memory[i] = '_';
    }

    if (shmdt(shared_memory) == -1) handle_error("shmdt");

    return 0;
}
