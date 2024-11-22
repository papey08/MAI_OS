#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <errno.h>

#define SHM_NAME "/shared_memory"
#define SHM_ERROR_NAME "/shared_error_memory"
#define SEM_PARENT_WRITE "/sem_parent_write"
#define SEM_CHILD_READ "/sem_child_read"
#define BUFFER_SIZE 1024

void print_error(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}

int main() {
    pid_t pid;
    char filename[BUFFER_SIZE];
    char input[BUFFER_SIZE];
    char *shared_memory;
    char *error_memory;

    const char start_msg[] = "Введите имя файла: ";
    write(STDOUT_FILENO, start_msg, sizeof(start_msg));
    int filename_len = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (filename_len <= 0) {
        print_error("Ошибка ввода имени файла\n");
        exit(EXIT_FAILURE);
    }
    filename[filename_len - 1] = '\0';

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    int shm_error_fd = shm_open(SHM_ERROR_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1 || shm_error_fd == -1) {
        print_error("Ошибка при создании shared memory\n");
        exit(EXIT_FAILURE);
    }
    ftruncate(shm_fd, BUFFER_SIZE);
    ftruncate(shm_error_fd, BUFFER_SIZE);
    shared_memory = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    error_memory = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_error_fd, 0);
    if (shared_memory == MAP_FAILED || error_memory == MAP_FAILED) {
        print_error("Ошибка при отображении shared memory\n");
        exit(EXIT_FAILURE);
    }

    sem_t *sem_parent_write = sem_open(SEM_PARENT_WRITE, O_CREAT, 0666, 0);
    sem_t *sem_child_read = sem_open(SEM_CHILD_READ, O_CREAT, 0666, 0);
    if (sem_parent_write == SEM_FAILED || sem_child_read == SEM_FAILED) {
        print_error("Ошибка при создании семафоров\n");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        print_error("Ошибка при создании процесса\n");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        execlp("./child", "./child", filename, (char *)NULL);
        print_error("Ошибка при запуске дочернего процесса\n");
        exit(EXIT_FAILURE);
    }

    const char msg1[] = "Введите строку (или 'exit' для выхода):\n";
    write(STDOUT_FILENO, msg1, sizeof(msg1));

    while (1) {
        int input_len = read(STDIN_FILENO, input, sizeof(input));
        if (input_len <= 0) {
            print_error("Ошибка ввода строки\n");
            break;
        }
        input[strcspn(input, "\n")] = 0;

        strncpy(shared_memory, input, BUFFER_SIZE);

        sem_post(sem_parent_write);

        if (strcmp(input, "exit") == 0) {
            break;
        }

        sem_wait(sem_child_read);

        if (strlen(error_memory) > 0) {
            write(STDERR_FILENO, error_memory, strlen(error_memory));
            memset(error_memory, 0, BUFFER_SIZE);
        }
    }

    wait(NULL);
    munmap(shared_memory, BUFFER_SIZE);
    munmap(error_memory, BUFFER_SIZE);
    shm_unlink(SHM_NAME);
    shm_unlink(SHM_ERROR_NAME);
    sem_close(sem_parent_write);
    sem_close(sem_child_read);
    sem_unlink(SEM_PARENT_WRITE);
    sem_unlink(SEM_CHILD_READ);

    return 0;
}


