#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define HEX_STRING_LENGTH 32
#define BUFFER_SIZE 64

uint64_t sum = 0;
size_t totalCount = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int activeThreads = 0;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
size_t maxThreads;
size_t memoryLimit;

typedef struct {
    uint64_t *numbers;
    size_t count;
} ThreadData;

void *processChunk(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    uint64_t localSum = 0;

    for (size_t i = 0; i < data->count; i++) {
        localSum += data->numbers[i];
    }

    pthread_mutex_lock(&mutex);
    sum += localSum;
    totalCount += data->count;
    activeThreads--;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    free(data->numbers);
    free(data);
    return NULL;
}

void createThreads(uint64_t *numbers, size_t count) {
    ThreadData *data = malloc(sizeof(ThreadData));
    data->numbers = numbers;
    data->count = count;

    pthread_t thread;
    pthread_mutex_lock(&mutex);
    while (activeThreads >= maxThreads) {
        pthread_cond_wait(&cond, &mutex);
    }
    activeThreads++;
    pthread_mutex_unlock(&mutex);

    if (pthread_create(&thread, NULL, processChunk, data) != 0) {
        _exit(1);
    }
    pthread_detach(thread);
}

void uint64ToStr(uint64_t value, char *buffer) {
    char temp[20];
    int i = 0;

    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\n';
        buffer[2] = '\0';
        return;
    }

    while (value > 0) {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    }

    int j = 0;
    while (i > 0) {
        buffer[j++] = temp[--i];
    }
    buffer[j++] = '\n';
    buffer[j] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        const char *msg = "Использование: <имя файла> <макс. потоки> <память>\n";
        write(STDERR_FILENO, msg, strlen(msg));
        _exit(1);
    }

    const char *filename = argv[1];
    maxThreads = strtoul(argv[2], NULL, 10);
    memoryLimit = strtoul(argv[3], NULL, 10);

    int file = open(filename, O_RDONLY);
    if (file == -1) {
        const char *msg = "Ошибка открытия файла\n";
        write(STDERR_FILENO, msg, strlen(msg));
        _exit(1);
    }

    size_t numbersPerChunk = memoryLimit / sizeof(uint64_t);
    char buffer[BUFFER_SIZE];
    size_t bufferIndex = 0;
    uint64_t *numbers = malloc(numbersPerChunk * sizeof(uint64_t));
    size_t count = 0;

    while (1) {
        ssize_t bytesRead = read(file, buffer + bufferIndex, BUFFER_SIZE - bufferIndex);
        if (bytesRead <= 0) break;

        bytesRead += bufferIndex;
        size_t start = 0;

        for (size_t i = 0; i < bytesRead; i++) {
            if (buffer[i] == '\n') {
                buffer[i] = '\0';
                numbers[count++] = strtoull(buffer + start, NULL, 16);
                start = i + 1;

                if (count == numbersPerChunk) {
                    createThreads(numbers, count);
                    numbers = malloc(numbersPerChunk * sizeof(uint64_t));
                    count = 0;
                }
            }
        }

        bufferIndex = bytesRead - start;
        memmove(buffer, buffer + start, bufferIndex);
    }

    if (count > 0) {
        createThreads(numbers, count);
    } else {
        free(numbers);
    }

    pthread_mutex_lock(&mutex);
    while (activeThreads > 0) {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    close(file);

    uint64_t average = (totalCount > 0) ? (sum / totalCount) : 0;

    char result[64];
    uint64ToStr(average, result);
    write(STDOUT_FILENO, "Среднее арифметическое: ", 45);
    write(STDOUT_FILENO, result, strlen(result));

    return 0;
}