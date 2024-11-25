#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <semaphore.h>
#include <string.h>

#define SHM_NAME "/shared_memory"
#define SEM_CHILD "/sem_child"
#define SEM_PARENT "/sem_parent"
#define BUFSIZE 1024

int reverse(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
    return 0;
}

float my_pow(float base, int exp) {
    float result = 1.0;
    int is_negative = 0;

    if (exp < 0) {
        exp = -exp;
        is_negative = 1;
    }

    while (exp > 0) {
        if (exp % 2 == 1) {
            result *= base;
        }
        base *= base;
        exp /= 2;
    }

    if (is_negative) {
        result = 1.0 / result;
    }
    return result;
}

int int_to_str(int num, char* str, int d) {
    int i = 0;
    int is_negative = 1 ? num < 0: 0;
    if (is_negative) {
        num *= -1;
    }
    if (num == 0) {
        str[i++] = '0';
    } else {
        while (num != 0) {
            str[i++] = (num % 10) + '0';
            num /= 10;
        }
    }

    while (i < d) {
        str[i++] = '0';  // добавляем нули, если нужно
    }

    if (is_negative) {
        str[i++] = '-';
    }
    reverse(str, i);
    str[i] = '\0';
    return i;
}

int float_to_str(float n, char* res, int afterpoint) {
    int ipart = (int)n;

    float fpart = n - (float)ipart;
    int i = int_to_str(ipart, res, 0);

    if (afterpoint != 0) {
        res[i] = '.';  
        i++;

        fpart = fpart * my_pow(10, afterpoint);

        int_to_str((int)fpart, res + i, afterpoint);
    }
    return 0;
}

int main() {

    const char start_msg[] = "Type name of your file: ";
    write(STDOUT_FILENO, start_msg, sizeof(start_msg));

    char filename[BUFSIZE];
    int n = read(STDIN_FILENO, filename, sizeof(filename) - 1);

    if (n > 0 && n < sizeof(filename)) {
        filename[n - 1] = '\0';
    } else {
        filename[sizeof(filename) - 1] = '\0';
    }

    int shm = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0666);
    if (shm == -1) {
        char* msg = "ERROR: fail to create shared memory\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(-1);
    }


    if (ftruncate(shm, sizeof(char) * 32) == -1) {
        char* msg = "ERROR: fail to set size of shared memory\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(-1);
    }

    sem_t* sem_child = sem_open(SEM_CHILD, O_CREAT, 0666, 0);
    if (sem_child == SEM_FAILED) {
        char* msg = "ERROR: fail to open semaphore\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(-1);
    }

    sem_t* sem_parent = sem_open(SEM_PARENT, O_CREAT, 0666, 0);
    if (sem_parent == SEM_FAILED) {
        char* msg = "fail to open semaphore\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(-1);
    }

    int fd[2]; 

    pid_t pid = fork();

    float* res = (float*)mmap(0, sizeof(float) * 2, PROT_READ|PROT_WRITE, MAP_SHARED, shm, 0);
    
    switch (pid)
    {
    case -1:
        const char msg[] = "ERROR: new process has not been created\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
        break;
    case 0:

        execlp("./child", "./child", filename, (char*)NULL);

        const char exec_msg[] = "ERROR: process has not started\n";
        write(STDERR_FILENO, exec_msg, sizeof(exec_msg));
        exit(EXIT_FAILURE);
        break;
    default:

        sem_post(sem_parent);
        sem_wait(sem_child);

        if (res[1] != -1.) {
            char str[BUFSIZE];
            float_to_str(*res, str, 5);
            size_t size_msg = strlen(str);
            str[size_msg] = '\n';
            write(STDOUT_FILENO, str, ++size_msg);
        } 

        break;
    }

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_CHILD);
    sem_unlink(SEM_PARENT);
    munmap(res, sizeof(float) * 2);
    sem_close(sem_child);
    sem_close(sem_parent);

    return 0;
}