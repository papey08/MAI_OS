
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <unistd.h>

#define ROWS 10
#define COLS 10

int matrix[ROWS][COLS];
int temp_matrix[ROWS][COLS];

typedef struct {
    int start_row;
    int end_row;
    int window_size;
    int iterations;
} ThreadArgs;

typedef enum {
    OK,
    INVALID_INPUT,
    SYSTEM_ERROR
} return_code;

sem_t semaphore;

int compare(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

int find_median(int* window, int size) {
    qsort(window, size, sizeof(int), compare);
    return window[size / 2];
}
void* median_philter(void* args) {
    ThreadArgs* thread_args = (ThreadArgs*)args;
    int start_row = thread_args->start_row;
    int end_row = thread_args->end_row;
    int window_size = thread_args->window_size;
    int iterations = thread_args->iterations;
    int offset = window_size / 2;

    for (int iter = 0; iter < iterations; iter++) {
        sem_wait(&semaphore);

        for (int i = start_row; i < end_row; i++) {
            for (int j = 0; j < COLS; j++) {
                if (i < offset || i >= ROWS - offset || j < offset || j >= COLS - offset) {
                    temp_matrix[i][j] = matrix[i][j]; 
                } else {
                    int window[window_size * window_size];
                    int idx = 0;
                    for (int wi = -offset; wi <= offset; wi++) {
                        for (int wj = -offset; wj <= offset; wj++) {
                            window[idx++] = matrix[i + wi][j + wj];
                        }
                    }
                    temp_matrix[i][j] = find_median(window, window_size * window_size);
                }
            }
        }

        sem_post(&semaphore);
    }
    return NULL;
}


void print_message(const char* message) {
    write(STDOUT_FILENO, message, sizeof(char) * strlen(message));
}


void copy_temp_to_matrix() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            matrix[i][j] = temp_matrix[i][j];
        }
    }
}

int is_number(const char* str) {
    while (*str) {
        if (*str < '0' || *str > '9') return 0;
        str++;
    }
    return INVALID_INPUT;
}

void generate_matrix() {
    srand(time(NULL));
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            matrix[i][j] = rand() % 100; 
        }
    }
}

void int_to_str(int num, char* buffer) {
    char temp[12];
    int i = 0, j = 0;

    if (num < 0) {
        buffer[j++] = '-';
        num = -num;
    }

    do {
        temp[i++] = (num % 10) + '0';
        num /= 10;
    } while (num > 0);

    while (i > 0) {
        buffer[j++] = temp[--i];
    }

    buffer[j] = '\0'; 
}

void print_matrix(int mat[ROWS][COLS]) {
    char buffer[64];
    for (int i = 0; i < ROWS; i++) {
        int offset = 0;
        for (int j = 0; j < COLS; j++) {
            char num_str[12];
            int_to_str(mat[i][j], num_str);
            int len = strlen(num_str);
            if (offset + len + 1 < sizeof(buffer)) {
                memcpy(buffer + offset, num_str, len);
                offset += len;

                buffer[offset++] = ' ';
            }
        }
        
        buffer[offset - 1] = '\n';
        
        write(STDOUT_FILENO, buffer, offset);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        char msg[] = "Usage: ./a.out <window_size> <iterations> <max_threads>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return INVALID_INPUT;
    }

    int window_size, iterations, max_threads;

    if (!is_number(argv[1]) || !is_number(argv[2]) || !is_number(argv[3])) {
        char msg[] = "all arguments must be positive integers.\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return INVALID_INPUT;
    }
    window_size = atoi(argv[1]);
    iterations = atoi(argv[2]);
    max_threads = atoi(argv[3]);

    if (window_size < 2) {
        char msg[] = "window size must be >= 2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return INVALID_INPUT;
    }

    if (iterations <= 0 || max_threads <= 0) {
        char msg[] = "mterations and threads count must be positive integers\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return INVALID_INPUT;
    }

    if (max_threads > ROWS) {
        char msg[] = "max threads cannot exceed the number of rows in the matrix\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return INVALID_INPUT;
    }

    if (sem_init(&semaphore, 0, max_threads) != 0) {
        char msg[] = "сouldn't initialize semaphore.\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return SYSTEM_ERROR;
    }

    generate_matrix();

    char msg[] = "original matrix:\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    print_matrix(matrix);

    pthread_t threads[max_threads];
    ThreadArgs thread_args[max_threads];
    int rows_per_thread = ROWS / max_threads;

    for (int i = 0; i < max_threads; i++) {
        thread_args[i].start_row = i * rows_per_thread;
        thread_args[i].end_row = (i == max_threads - 1) ? ROWS : (i + 1) * rows_per_thread;
        thread_args[i].window_size = window_size;
        thread_args[i].iterations = iterations;

        if (pthread_create(&threads[i], NULL, median_philter, &thread_args[i]) != 0) {
            char msg[] = "couldn't create thread.\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            return SYSTEM_ERROR;
        }
    }

    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < max_threads; i++) {
            sem_post(&semaphore); 
        }

        for (int i = 0; i < max_threads; i++) {
            sem_wait(&semaphore); 
        }

        copy_temp_to_matrix();
    }
    for (int i = 0; i < max_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            char msg[] = "couldn't join thread\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            return SYSTEM_ERROR;
        }
    }

    char final_msg[] = "result matrix:\n";
    write(STDOUT_FILENO, final_msg, sizeof(final_msg) - 1);
    print_matrix(matrix);

    sem_destroy(&semaphore);

    return OK;
}