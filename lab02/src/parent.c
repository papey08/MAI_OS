#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>


#define SHM_SIZE 1024

void handle_error(const char *msg) {
    const char *error_message = ": Ошибка\n";
    write(2, msg, strlen(msg));
    write(2, error_message, strlen(error_message));
    _exit(EXIT_FAILURE);
}

void write_message(const char *msg) {
    write(1, msg, strlen(msg));
}

void read_message(char *buffer, size_t size) {
    ssize_t bytes_read = read(0, buffer, size - 1);
    if (bytes_read <= 0) handle_error("Ошибка чтения");
    buffer[bytes_read - 1] = '\0';
}

int main() {
    // Create and open shared memory using shm_open
    int fd = shm_open("/shared_memory", O_CREAT | O_RDWR, 0666);
    if (fd == -1) handle_error("Ошибка создания файла");

    // Set the size of the shared memory object
    if (ftruncate(fd, SHM_SIZE) == -1) handle_error("ftruncate");

    // Map shared memory into process's address space
    char *shared_memory = (char *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_memory == MAP_FAILED) handle_error("mmap");

    // Create semaphores
    sem_t *sem_parent = sem_open("/sem_parent", O_CREAT, 0666, 0);
    sem_t *sem_child1 = sem_open("/sem_child1", O_CREAT, 0666, 0);
    sem_t *sem_child2 = sem_open("/sem_child2", O_CREAT, 0666, 0);

    if (sem_parent == SEM_FAILED || sem_child1 == SEM_FAILED || sem_child2 == SEM_FAILED)
        handle_error("sem_open");

    pid_t pid1 = fork();
    if (pid1 == -1) handle_error("fork");

    if (pid1 == 0) {
        execl("./child1", "./child1", NULL);
        handle_error("execl (child1)");
    }

    pid_t pid2 = fork();
    if (pid2 == -1) handle_error("fork");

    if (pid2 == 0) {
        execl("./child2", "./child2", NULL);
        handle_error("execl (child2)");
    }

    write_message("Введите строку (или пустую строку для выхода): ");
    char input_buffer[SHM_SIZE];

    while (1) {
        read_message(input_buffer, SHM_SIZE);

        if (strcmp(input_buffer, "") == 0) {
            break;
        }

        strcpy(shared_memory, input_buffer);

        sem_post(sem_child1);
        sem_wait(sem_parent);

        sem_post(sem_child2);
        sem_wait(sem_parent);

        write_message("Результат обработки: ");
        write_message(shared_memory);
        write_message("\nВведите строку (или пустую строку для выхода): ");
    }

    strcpy(shared_memory, "");
    sem_post(sem_child1);
    sem_post(sem_child2);

    wait(NULL);
    wait(NULL);

    // Cleanup
    if (munmap(shared_memory, SHM_SIZE) == -1) handle_error("munmap");
    if (shm_unlink("/shared_memory") == -1) handle_error("shm_unlink");

    if (sem_close(sem_parent) == -1) handle_error("sem_close sem_parent");
    if (sem_close(sem_child1) == -1) handle_error("sem_close sem_child1");
    if (sem_close(sem_child2) == -1) handle_error("sem_close sem_child2");

    if (sem_unlink("/sem_parent") == -1) handle_error("sem_unlink sem_parent");
    if (sem_unlink("/sem_child1") == -1) handle_error("sem_unlink sem_child1");
    if (sem_unlink("/sem_child2") == -1) handle_error("sem_unlink sem_child2");

    return 0;
}
