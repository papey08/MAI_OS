#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/stat.h>

#define SHM_NAME "/shared_memory"
#define SEM_REQUEST "/sem_request"
#define SEM_RESPONSE "/sem_response"

typedef struct {
    int number;
    int signal;
    char filename[128];
} SharedData;

void handle_error(const char *message) {
    write(STDERR_FILENO, message, strlen(message));
    exit(EXIT_FAILURE);
}

int main() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        handle_error("Ошибка при создании разделяемой памяти\n");
    }

    if (ftruncate(shm_fd, sizeof(SharedData)) == -1) {
        handle_error("Ошибка при установке размера разделяемой памяти\n");
    }

    SharedData *data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) {
        handle_error("Ошибка при отображении разделяемой памяти\n");
    }

    sem_t *sem_request = sem_open(SEM_REQUEST, O_CREAT, 0666, 0);
    sem_t *sem_response = sem_open(SEM_RESPONSE, O_CREAT, 0666, 0);

    if (sem_request == SEM_FAILED || sem_response == SEM_FAILED) {
        handle_error("Ошибка при создании семафоров\n");
    }

    write(STDOUT_FILENO, "Введите имя файла: ", 34);
    int n = read(STDIN_FILENO, data->filename, sizeof(data->filename));

    if (n <= 0) {
        handle_error("Ошибка при чтении имени файла\n");
    }
    data->filename[n - 1] = '\0';

    pid_t pid = fork();
    if (pid > 0) {
        char input[128];
        while (1) {
            write(STDOUT_FILENO, "Введите число: ", 28);
            n = read(STDIN_FILENO, input, sizeof(input));
            if (n <= 0) {
                handle_error("Ошибка при чтении числа\n");
            }

            input[n - 1] = '\0';
            char *endptr;
            data->number = strtol(input, &endptr, 10);
            if (endptr == input || *endptr != '\0') {
                handle_error("Ошибка: вводите только числа\n");
            }

            sem_post(sem_request);
            sem_wait(sem_response);

            if (data->signal == 1) {
                write(STDOUT_FILENO, "Программа завершена.\n", 40);
                break;
            }
        }

        wait(NULL);
        munmap(data, sizeof(SharedData));
        shm_unlink(SHM_NAME);
        sem_close(sem_request);
        sem_close(sem_response);
        sem_unlink(SEM_REQUEST);
        sem_unlink(SEM_RESPONSE);
    } else if (pid == 0) {
        execl("./child", "./child", NULL);
        handle_error("Ошибка при запуске дочернего процесса\n");
    } else {
        handle_error("Ошибка при вызове fork\n");
    }

    return 0;
}