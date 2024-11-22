#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>

#define SHM_NAME "/shared_memory"
#define SHM_ERROR_NAME "/shared_error_memory"
#define SEM_PARENT_WRITE "/sem_parent_write"
#define SEM_CHILD_READ "/sem_child_read"
#define BUFFER_SIZE 1024

void report_error(const char *error_memory, const char *msg) {
    strncpy((char *)error_memory, msg, BUFFER_SIZE);
}

int ends_with_dot_or_semicolon(const char *str) {
    int len = strlen(str);
    return (len > 0 && (str[len - 1] == '.' || str[len - 1] == ';'));
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        write(STDERR_FILENO, "Не указано имя файла\n", 22);
        return -1;
    }

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    int shm_error_fd = shm_open(SHM_ERROR_NAME, O_RDWR, 0666);
    if (shm_fd == -1 || shm_error_fd == -1) {
        write(STDERR_FILENO, "Ошибка при открытии shared memory\n", 34);
        return -1;
    }
    char *shared_memory = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    char *error_memory = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_error_fd, 0);
    if (shared_memory == MAP_FAILED || error_memory == MAP_FAILED) {
        write(STDERR_FILENO, "Ошибка при отображении shared memory\n", 38);
        return -1;
    }

    sem_t *sem_parent_write = sem_open(SEM_PARENT_WRITE, 0);
    sem_t *sem_child_read = sem_open(SEM_CHILD_READ, 0);
    if (sem_parent_write == SEM_FAILED || sem_child_read == SEM_FAILED) {
        write(STDERR_FILENO, "Ошибка при открытии семафоров\n", 31);
        return -1;
    }

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        report_error(error_memory, "Ошибка открытия файла\n");
        sem_post(sem_child_read);
        return -1;
    }

    while (1) {
        sem_wait(sem_parent_write);

        char buffer[BUFFER_SIZE];
        strncpy(buffer, shared_memory, BUFFER_SIZE);

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        if (ends_with_dot_or_semicolon(buffer)) {
            write(fd, buffer, strlen(buffer));
            write(fd, "\n", 1);
        } else {
            report_error(error_memory, "Строка не соответствует правилу!\n");
        }

        sem_post(sem_child_read);
    }

    close(fd);
    munmap(shared_memory, BUFFER_SIZE);
    munmap(error_memory, BUFFER_SIZE);
    sem_close(sem_parent_write);
    sem_close(sem_child_read);

    return 0;
}



