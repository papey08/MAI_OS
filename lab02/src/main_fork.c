#include <stdlib.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>

#include <sys/mman.h>

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


void lltostr(long long a, char **ptr) {
    if (a == 0) {
        **ptr = '0';
        (*ptr)[1] = 0;
        return;
    }
    if (a < 0) {
        **ptr = '-';
        ++(*ptr);
    }
    a = llabs(a);
    if (a > 0) {
        lltostr(a / 10, ptr);
        **ptr = '0' + (a % 10);
        ++(*ptr);
    }
    **ptr = 0;
    return;
}


void swap(long long *a, long long *b) {
    long long buff = *a;
    *a = *b;
    *b = buff;
}


void check_if_sorted(long long *array, long long n) {
    long long *ptr = array;
    for (long long i = 1; i < n; ++i) {
        if (*ptr > ptr[1]) {
            char msg[] = "not sorted\n";            
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            return;
        }
        ++ptr;
    }
    char msg[] = "sorted\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    return;
}


void sort(long long **left, long long **right, long long **seed, long long *begin, long long *end) {
    while (*left < *right) {
        if ((**left >= **seed) && (**right < **seed)) {
            swap(*left, *right);
        }
        while (**left < **seed) {
            ++(*left);
        }
        while (*right >= begin && **right >= **seed) {
            --(*right);
        }
    }
    if (**left > **seed) {
        swap(*left, *seed);
    }
    *seed = *left;

    while (**left == **seed) {
        ++(*left);
    }
    
    ++(*right);

    return;
}


int my_qsort(long long *array, long long *begin, long long *end, int *running_threads_count, int max_threads_count) {
    if (end - begin < 2) {
        return 0;
    }
    long long *seed = end - 1;
    long long *left = begin;
    long long *right = end - 2;

    sort(&left, &right, &seed, begin, end);
   
    if (((end - left > 1e5)) && (*(running_threads_count) < max_threads_count)) {
        (*(running_threads_count))++;
        pid_t child = fork();
        switch (child)
        {
        case -1:
            char msg[] = "error unlocking the mutex\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            return 1;
            break;
        
        case 0:
            my_qsort(array, left, end, running_threads_count, max_threads_count);
            (*running_threads_count)--;
            exit(0);

        default:
            my_qsort(array, begin, right, running_threads_count, max_threads_count);
            break;
        }
        
    } else {
        my_qsort(array, left, end, running_threads_count, max_threads_count);
        my_qsort(array, begin, right, running_threads_count, max_threads_count);
    }

    return 0;
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        char msg[] = "wrong number of args\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return -1;
    }

    int max_threads_count = atoi(argv[1]);
   
    int *running_threads_count;

    int shmid;
    char pathname[] = "main.c"; 
    key_t key;

    if((key = ftok(pathname, 0)) < 0) {
        char msg[] = "ftok error\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        return -1;
    }
    
    if((shmid = shmget(key, sizeof(int), 0666|IPC_CREAT|IPC_EXCL)) < 0) {
        if (errno = EEXIST) {
            if (shmctl(shmid, 0, NULL) < 0) {
                char msg[] = "shmctl error\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                return -1;
            }
            if((shmid = shmget(key, sizeof(int), 0666|IPC_CREAT|IPC_EXCL)) < 0) {
                char msg[] = "shmget error\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                return -1;
            }
        } else {
            char msg[] = "shmget error\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            return -1;
        }
    }
    
    if((running_threads_count = (int *)shmat(shmid, NULL, 0)) == (int *)(-1)) {
        char msg[] = "shmat\n";
        write(STDERR_FILENO, msg, sizeof(msg));

        if (shmctl(shmid, 0, NULL) < 0) {
            char msg[] = "shmctl error\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            return -1;
        }

        return -1;
    }

    (*running_threads_count) = 1;

    long long n = (long long)(1e8);

    int fd;
    if ((fd = open("test.bin", O_RDWR)) < 0) {
        char msg[] = "file error\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);

        if(shmdt(running_threads_count) < 0) { 
            char msg[] = "shmdt error\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            return -1;
        }
        if (shmctl(shmid, 0, NULL) < 0) {
            char msg[] = "shmctl error\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            return -1;
        }

        return -1;
    }

    // long long n = 6;
    // long long array[6] = {1LL, 5LL, 9LL, 2LL, 8LL, 13LL};

    long long *array;
    if ((array = mmap(NULL, n * sizeof(long long), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == (void *)(-1)) {
        char msg[] = "mmap error\n";
        write(STDERR_FILENO, msg, sizeof(msg));

        close(fd);
        if(shmdt(running_threads_count) < 0) { 
            char msg[] = "shmdt error\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            return -1;
        }
        if (shmctl(shmid, 0, NULL) < 0) {
            char msg[] = "shmctl error\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            return -1;
        }

        return -1;
    }   

    time_start();
    write(STDOUT_FILENO, "started\n", sizeof("started\n") - 1);
    my_qsort(array, array, array + n, running_threads_count, max_threads_count);
    (*running_threads_count)--;
    while (*running_threads_count > 0) {
        wait(0);
    }
    long long time = (long long)time_stop();

    write(STDOUT_FILENO, "completed in ", sizeof("completed in ") - 1);

    char *strtime = (char *)malloc(25 * sizeof(char));
    if (strtime == NULL) {
        char msg[] = "error allocating memory\n";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);

        munmap(array, n * sizeof(long long));
        close(fd);
        if(shmdt(running_threads_count) < 0) { 
            char msg[] = "shmdt error\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            return -1;
        }
        if (shmctl(shmid, 0, NULL) < 0) {
            char msg[] = "shmctl error\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            return -1;
        }
        
        return 1;
    }
    char *ptr = strtime;
    lltostr(time, &ptr);
    write(STDOUT_FILENO, strtime, sizeof(strtime) - 1);
    write(STDOUT_FILENO, "ms\n", sizeof("ms\n") - 1);
    free(strtime);

    check_if_sorted(array, n);
    //msync(array,  n * sizeof(long long), MS_SYNC);
    free(running_threads_count);
    munmap(array, n * sizeof(long long));
    close(fd);
    if(shmdt(running_threads_count) < 0) { 
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
