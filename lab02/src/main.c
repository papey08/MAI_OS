#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/shm.h>
#include <sys/ipc.h>

#include <sys/time.h>
struct timeval tv1,tv2,dtv;
struct timezone tz;
void time_start() { gettimeofday(&tv1, &tz); }
long time_stop()
{ gettimeofday(&tv2, &tz);
  dtv.tv_sec= tv2.tv_sec -tv1.tv_sec;
  dtv.tv_usec=tv2.tv_usec-tv1.tv_usec;
  if(dtv.tv_usec<0) { dtv.tv_sec--; dtv.tv_usec+=1000000; }
  return dtv.tv_sec*1000+dtv.tv_usec/1000;
}


void swap(long long *a, long long *b) {
    long long buff = *a;
    *a = *b;
    *b = buff;
}


void my_qsort(long long *begin, long long *end, int max_threads_count, long long *running_threads, long long *array_begin) {
    if (end - begin < 2) {
        return;
    }
    long long *seed = end - 1;
    long long *left = begin;
    long long *right = end - 2;

    while (left < right) {
        if ((*left >= *seed) && (*right < *seed)) {
            swap(left, right);
        }
        while (*left < *seed) {
            ++left;
        } 
        while (right >= begin && *right >= *seed) {
            --right;
        }
    }
    
    if (*left > *seed) {
        swap(left, seed);
    }
    seed = left;

    while (*left == *seed) {
        ++left;
    }
    ++right;
    
    if ((*running_threads < max_threads_count) && (end - begin > (int)1e6)) {
        (*running_threads)++;
        pid_t child = fork();
        switch (child)
        {
        case -1:
            char msg[] = "fork error\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            (*running_threads)--;
            break;
        case 0:            
            my_qsort(left, end, max_threads_count, running_threads, array_begin);
            (*running_threads)--; 
            if(shmdt(array_begin) < 0) { 
                char msg[] = "shmdt error\n";
                write(STDERR_FILENO, msg, sizeof(msg));
            }
            exit(0);
            break;

        default:
            my_qsort(begin, right, max_threads_count, running_threads, array_begin);
            break;
        }
    } else {
        my_qsort(begin, right, max_threads_count, running_threads, array_begin);
        my_qsort(left, end, max_threads_count, running_threads, array_begin);
    }
    
    return;
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        char msg[] = "wrong number of args\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        return -1;
    }

    int max_threads_count = atoi(argv[1]);
    
    FILE* file;
    if ((file = fopen("test.txt", "r")) == NULL) {
        char msg[] = "file error\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        return -1;
    }
    long long n;
    fscanf(file, "%lld", &n);

    long long *array;   
    int shmid;
    char pathname[] = "main.c"; 
    key_t key;

    if((key = ftok(pathname, 0)) < 0) {
        char msg[] = "ftok error\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        fclose(file);
        return -1;
    }
    
    if((shmid = shmget(key, (n + 1) * sizeof(long long), 0666|IPC_CREAT|IPC_EXCL)) < 0) {
        char msg[] = "shmged error\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        fclose(file);
        return -1;
    }
    
    if((array = (long long *)shmat(shmid, NULL, 0)) == (long long *)(-1)) {
        char msg[] = "shmat\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        fclose(file);
        return -1;
    }

    for (long long i = 0; i < n; ++i) {
        fscanf(file, "%lld", &array[i]);
    }
    fclose(file);

    long long *running_threads = &(array[n]);
    (*running_threads) = 1;

    time_start();
    printf("%d started...\n", getpid());  
    my_qsort(array, array + n, max_threads_count, running_threads, array);
    while (*running_threads > 1) {
        wait(0);
    }
    printf("%d completed in %ldms\n", getpid(), time_stop());

    if ((file = fopen("test_out.txt", "w")) == NULL) {
        char msg[] = "file error\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        return -1;
    }

    for (long long i = 0; i < n; ++i) {
        fprintf(file, "%lld\n", array[i]);
    }
    putc('\n', file);
    fclose(file);

    if(shmdt(array) < 0) { 
        char msg[] = "shmdt error\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        return -1;
    }
    if (shmctl(shmid, 0, NULL) < 0) {
        char msg[] = "shmctl error\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        return -1;
    }
    return 0;
}
