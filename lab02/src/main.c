#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define MAX_N 10
#define MAX_LINE_LENGTH 100

double matrix[MAX_N][MAX_N + 1];
int n, numThreads;
pthread_mutex_t mutex;
pthread_cond_t cond;
int counter = 0;

void IntToString(int value, char* buffer) {
    int i = 0;
    int isNegative = (value < 0);

    if (isNegative) {
        value = -value;
    }

    do {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    } while (value);

    if (isNegative) {
        buffer[i++] = '-';
    }
    buffer[i] = '\0';

    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }
}

void DoubleToString(double value, char* buffer) {
    int intPart = (int)value;
    double fracPart = value - intPart;
    IntToString(intPart, buffer);

    int len = strlen(buffer);
    buffer[len++] = '.';

    for (int i = 0; i < 6; i++) {
        fracPart *= 10;
        int digit = (int)fracPart;
        buffer[len++] = digit + '0';
        fracPart -= digit;
    }
    buffer[len] = '\0';
}

void WriteString(const char* str) {
    write(1, str, strlen(str));
}

void WriteDouble(double value) {
    char buffer[50];
    DoubleToString(value, buffer);
    WriteString(buffer);
}

void WriteInt(int value) {
    char buffer[12];
    IntToString(value, buffer);
    WriteString(buffer);
}

void ReadLine(char* buffer, int size) {
    read(0, buffer, size);
    buffer[strcspn(buffer, "\n")] = 0; // Удаляем символ новой строки
}

void ReadDoubles(double* values, int count) {
    char line[MAX_LINE_LENGTH];
    ReadLine(line, sizeof(line));

    char* token = strtok(line, " ");
    for (int i = 0; i < count; i++) {
        if (token != NULL) {
            values[i] = atof(token);
            token = strtok(NULL, " ");
        }
    }
}

void WaitForSync() {
    pthread_mutex_lock(&mutex);
    counter++;
    if (counter < numThreads) {
        pthread_cond_wait(&cond, &mutex);
    } else {
        counter = 0;
        pthread_cond_broadcast(&cond);
    }
    pthread_mutex_unlock(&mutex);
}

void* GaussianElimination(void* arg) {
    int threadId = *(int*)arg;
    for (int k = 0; k < n; k++) {
        if (k % numThreads == threadId) {
            pthread_mutex_lock(&mutex);
            double pivot = matrix[k][k];
            for (int j = k; j <= n; j++) {
                matrix[k][j] /= pivot;
            }
            pthread_mutex_unlock(&mutex);
        }

        WaitForSync();

        for (int i = k + 1; i < n; i++) {
            if (i % numThreads == threadId) {
                pthread_mutex_lock(&mutex);
                double factor = matrix[i][k];
                for (int j = k; j <= n; j++) {
                    matrix[i][j] -= factor * matrix[k][j];
                }
                pthread_mutex_unlock(&mutex);
            }
        }

        WaitForSync();
    }
    return NULL;
}

void BackSubstitution(double* result) {
    for (int i = n - 1; i >= 0; i--) {
        result[i] = matrix[i][n];
        for (int j = i + 1; j < n; j++) {
            result[i] -= matrix[i][j] * result[j];
        }
    }
}

int main() {
    char inputLine[MAX_LINE_LENGTH];
    
    WriteString("Введите количество переменных (n <= 10): ");
    ReadLine(inputLine, sizeof(inputLine));
    n = atoi(inputLine);

    WriteString("Введите количество потоков: ");
    ReadLine(inputLine, sizeof(inputLine));
    numThreads = atoi(inputLine);

    if (numThreads <= 0 || numThreads > MAX_N) {
        WriteString("Неверное количество потоков. Установлено значение по умолчанию: 4\n");
        numThreads = 4;
    }

    WriteString("Введите элементы расширенной матрицы: \n");
    for (int i = 0; i < n; i++) {
        ReadDoubles(matrix[i], n + 1);
    }

    pthread_t threads[MAX_N];
    int threadIds[MAX_N];
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    for (int i = 0; i < numThreads; i++) {
        threadIds[i] = i;
        pthread_create(&threads[i], NULL, GaussianElimination, &threadIds[i]);
    }

    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    double result[MAX_N];
    BackSubstitution(result);

    WriteString("Решение системы:\n");
    for (int i = 0; i < n; i++) {
        WriteString("x");
        WriteInt(i + 1);
        WriteString(" = ");
        WriteDouble(result[i]);
        WriteString("\n");
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}
