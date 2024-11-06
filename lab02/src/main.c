#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct Matrix {
    float** matrix;
    int rows;
    int columns;
} Matrix;

typedef struct ThreadData {
    float** input;
    float** kernel;
    float** output;
    int start_row;
    int end_row;
    int rows;
    int cols;
    int kernel_size;
    int id;
} ThreadData;

float** allocate_matrix(int rows, int cols) {
    float** matrix = (float**)malloc(rows * sizeof(float*));
    for (int i = 0; i < rows; i++) {
        matrix[i] = (float*)calloc(cols, sizeof(float));
    }
    return matrix;
}

Matrix* CreateMatrix(int rows, int cols) {
    Matrix* m = (Matrix*)malloc(sizeof(Matrix));
    m->matrix = allocate_matrix(rows + 2, cols + 2);
    m->rows = rows + 2;
    m->columns = cols + 2;
    return m;
}

void FreeMatrix(Matrix* m) {
    for (int i = 0; i < m->rows; i++) {
        free(m->matrix[i]);
    }
    free(m->matrix);
    free(m);
}

Matrix* ProcessFile(const char* filename) {
    int file = open(filename, O_RDONLY);
    if (file < 0) {
        char msg[] = "Ошибка: не удалось открыть файл.\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return NULL;
    }

    int rows, cols;
    char buffer[100];
    int len = read(file, buffer, sizeof(buffer));
    buffer[len] = '\0';
    sscanf(buffer, "%d %d", &rows, &cols);

    Matrix* m = CreateMatrix(rows, cols);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            len = read(file, buffer, sizeof(buffer));
            buffer[len] = '\0';
            sscanf(buffer, "%f", &m->matrix[i + 1][j + 1]);
        }
    }
    close(file);
    return m;
}

void* ApplyConvolutionThread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int offset = data->kernel_size / 2;
    pthread_mutex_lock(&print_mutex);
    char msg[100];
    snprintf(msg, sizeof(msg), "Работает %d поток с %d по %d ряд\n", data->id, data->start_row, data->end_row);
    write(STDOUT_FILENO, msg, strlen(msg));
    pthread_mutex_unlock(&print_mutex);

    for (int i = data->start_row; i < data->end_row; i++) {
        for (int j = offset; j < data->cols - offset; j++) {
            float sum = 0.0;
            int row_offset = i - offset;
            int col_offset = j - offset;
            for (int ki = 0; ki < data->kernel_size; ki++) {
                for (int kj = 0; kj < data->kernel_size; kj++) {
                    sum += data->input[row_offset + ki][col_offset + kj] * data->kernel[ki][kj];
                }
            }
            data->output[i][j] = sum;
        }
    }
    pthread_mutex_lock(&print_mutex);
    snprintf(msg, sizeof(msg), "%d поток закончил работу\n", data->id);
    write(STDOUT_FILENO, msg, strlen(msg));
    pthread_mutex_unlock(&print_mutex);
    return NULL;
}

void save_matrix_to_file(float** matrix, int rows, int cols, const char* filename) {
    int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file < 0) {
        char msg[] = "Ошибка: не удалось сохранить файл.\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return;
    }
    char buffer[50];
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < cols - 1; j++) {
            int len = snprintf(buffer, sizeof(buffer), "%6.2f ", matrix[i][j]);
            write(file, buffer, len);
        }
        write(file, "\n", 1);
    }
    close(file);
}

void ApplyConvolution(float** input, float** kernel, float** output, int rows, int cols, int kernel_size, int num_threads) {
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];

    int offset = kernel_size / 2;
    int rows_per_thread = (rows - 2 * offset) / num_threads;
    int extra_rows = (rows - 2 * offset) % num_threads;

    for (int t = 0; t < num_threads; t++) {
        thread_data[t].id = t;
        thread_data[t].input = input;
        thread_data[t].kernel = kernel;
        thread_data[t].output = output;
        thread_data[t].rows = rows;
        thread_data[t].cols = cols;
        thread_data[t].kernel_size = kernel_size;
        thread_data[t].start_row = t * rows_per_thread + offset;
        thread_data[t].end_row = (t + 1) * rows_per_thread + offset;

        if (t == num_threads - 1) {
            thread_data[t].end_row += extra_rows;
        }

        pthread_create(&threads[t], NULL, ApplyConvolutionThread, &thread_data[t]);
    }

    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        char msg[] = "Usage: ./main <count> <kernel size> <max_threads>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }
    int K = atoi(argv[1]);
    int kernel_size = atoi(argv[2]);
    int count_thread = atoi(argv[3]);

    Matrix* m = ProcessFile("gen.txt");
    clock_t start, end;
    double cpu_time_used;
    start = clock();
    if (kernel_size > m->rows || kernel_size > m->columns || kernel_size % 2 == 0) {
        char msg[] = "Некорректный размер окна свёртки.\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return -1;
    }

    float** kernel = allocate_matrix(kernel_size, kernel_size);
    for (int i = 0; i < kernel_size; i++) {
        for (int j = 0; j < kernel_size; j++) {
            kernel[i][j] = 1.0 / (kernel_size * kernel_size);
        }
    }

    float** output = allocate_matrix(m->rows, m->columns);

    for (int k = 0; k < K; k++) {
        ApplyConvolution(m->matrix, kernel, output, m->rows, m->columns, kernel_size, count_thread);

        float** temp = m->matrix;
        m->matrix = output;
        output = temp;
    }
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("%lf\n", cpu_time_used);
    save_matrix_to_file(m->matrix, m->rows, m->columns, "output_matrix.txt");
    pthread_mutex_destroy(&print_mutex);
    FreeMatrix(m);
    free(output);
    free(kernel);

    return 0;
}
