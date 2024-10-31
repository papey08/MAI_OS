#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>

#define MAX_THREADS 10

typedef struct {
    int mod;
    int N;
    char* file;
    char* pattern;
    int pattern_len;
    unsigned int** res;
    int* res_size;
    int* counter;
    pthread_mutex_t* mutex;
} Data;

int uint_cmp(const void* a1, const void* a2) {
    return (*(unsigned int*)a1 > *(int*)a2) - (*(unsigned int*)a1 < *(int*)a2);
}

int print_unsigned_int(const unsigned int num) {
    char buf[16];
    char res[32];
    int n = 0;
    int x = num;
    if (x == 0) {
        write(STDOUT_FILENO, "0\n", 2);
        return 0;
    }
    while (x) {
        buf[n] = (x % 10) + '0';
        x = x / 10;
        n++;
    }
    for (int i = 0; i < n; i++) {
        res[i] = buf[n - i - 1];
    }
    res[n] = '\n';
    write(STDOUT_FILENO, res, (n + 1));
    return 0;
}

int str_to_int(const char* string) {
    int num = 0;
    int i = 0;
    char c = string[i];
    while ((c >= '0') && (c <= '9')) {
        if ((num * 10 + (c - '0')) > 100) {
            return -1;
        }
        num = num * 10 + (c - '0');
        i++;
        c = string[i];
    }
    if ((c == 0) && (i != 0)) {
        return num;
    } else {
        return -1;
    }
}

void* find_inclusions_by_mod(void* data) {
    Data* d = (Data*)data;

    int fd = open(d->file, O_RDONLY);
    if (fd == -1) {
        char* msg = "fail to open file\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        return NULL;
    }

    lseek(fd, d->mod, SEEK_SET);
    char* buf = malloc((d->pattern_len + 1) * sizeof(char));
    buf[d->pattern_len] = '\0';
    int index;
    int symb = d->mod;
    while (read(fd, buf, d->pattern_len) == d->pattern_len) {
        if (!strcmp(buf, d->pattern)) {
            pthread_mutex_lock(d->mutex);
            index = *(d->counter);
            if (index + 1 == *(d->res_size)) {
                *(d->res_size) *= 2;
                *(d->res) = (unsigned int*)malloc(*(d->res_size) * sizeof(unsigned int));
            }
            (*(d->counter))++;
            pthread_mutex_unlock(d->mutex);
            (*(d->res))[index] = symb;
        }
        lseek(fd, d->N - d->pattern_len, SEEK_CUR);
        symb += d->N;
    }
    close(fd);
    return NULL;
}

int main(int argc, char* argv[]) {
    
    if (argc != 4) {
        char* msg = "enter: ./main.out <path to file> <number of threads> <pattern>\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        return -1;
    }

    int N = str_to_int(argv[2]);
    if((N == -1) || (N > MAX_THREADS)) {
        char* msg = "invalid number of threads\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        return -1;
    }

    pthread_t* threads = (pthread_t*)malloc(N * sizeof(pthread_t));
    Data* threads_data = (Data*)malloc(N * sizeof(Data));
    char* file = (char*)malloc((strlen(argv[1]) + 1) * sizeof(char));
    strcpy(file, argv[1]);
    int pattern_len = strlen(argv[3]);
    char* pattern = (char*)malloc((pattern_len + 1) * sizeof(char));
    strcpy(pattern, argv[3]);
    int res_size = 256;
    unsigned int* results = (unsigned int*)malloc(res_size * sizeof(unsigned int));
    int counter = 0;

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    
    for (int i = 0; i < N; i++) {
        threads_data[i].file = file;
        threads_data[i].mod = i;
        threads_data[i].N = N;
        threads_data[i].pattern = pattern;
        threads_data[i].pattern_len = pattern_len;
        threads_data[i].res = &results;
        threads_data[i].res_size = &res_size;
        threads_data[i].counter = &counter;
        threads_data[i].mutex = &mutex;  
    }
    
    clock_t start = clock();
    for (int i = 0; i < N; i++) {
        pthread_create(threads + i, NULL, &find_inclusions_by_mod, threads_data + i);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }
    clock_t end = clock();

    qsort(results, counter, sizeof(int), &uint_cmp);

    for (int i = 0; i < counter; i++) {
        print_unsigned_int(results[i]);
    }
    write(STDOUT_FILENO, "time: ", 7);
    print_unsigned_int(end - start);

    free(threads);
    free(threads_data);
    free(pattern);
    free(results);
    
    pthread_mutex_destroy(&mutex);
    return 0;
}