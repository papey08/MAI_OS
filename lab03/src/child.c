#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
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

int is_prime(int num) {
    if (num < 2) {
        return 0;
    }
    for (int i = 2; i * i <= num; i++) {
        if (num % i == 0) {
            return 0;
        }
    }
    return 1;
}

int int_to_str(int num, char *buf, int buf_size) {
    int len = 0;
    int temp = num;

    if (num < 0) {
        if (buf_size > 1) {
            buf[len++] = '-';
            num = -num;
        } else {
            return -1;
        }
    }

    do {
        temp /= 10;
        len++;
    } while (temp > 0);

    if (len >= buf_size) {
        return -1;
    }

    buf[len] = '\0';
    while (num > 0) {
        buf[--len] = (num % 10) + '0';
        num /= 10;
    }

    return strlen(buf);
}

int main() {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        handle_error("Ошибка при открытии разделяемой памяти\n");
    }

    SharedData *data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) {
        handle_error("Ошибка при отображении разделяемой памяти\n");
    }

    sem_t *sem_request = sem_open(SEM_REQUEST, 0);
    sem_t *sem_response = sem_open(SEM_RESPONSE, 0);

    if (sem_request == SEM_FAILED || sem_response == SEM_FAILED) {
        handle_error("Ошибка при открытии семафоров\n");
    }

    int file_fd = open(data->filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_fd == -1) {
        handle_error("Ошибка при открытии файла\n");
    }

    while (1) {
        sem_wait(sem_request);

        int number = data->number;
        if (number < 0 || is_prime(number)) {
            data->signal = 1;
            sem_post(sem_response);
            break;
        } else {
            char buf[128];
            int len = int_to_str(number, buf, sizeof(buf));
            if (len == -1) {
                handle_error("Ошибка при преобразовании числа в строку\n");
            }

            if (write(file_fd, buf, len) == -1) {
                handle_error("Ошибка при записи в файл\n");
            }

            if (write(file_fd, " ", 1) == -1) {
                handle_error("Ошибка при записи пробела в файл\n");
            }

            data->signal = 0;
            sem_post(sem_response);
        }
    }

    close(file_fd);
    munmap(data, sizeof(SharedData));
    return 0;
}