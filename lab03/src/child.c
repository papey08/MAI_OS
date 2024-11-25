#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <semaphore.h>

#define SHM_NAME "/shared_memory"
#define SEM_CHILD "/sem_child"
#define SEM_PARENT "/sem_parent"
#define BUFSIZE 1024

int is_separator(const char c) {
    return (c == ' ') || (c == '\n') || (c == '\t') || (c == '\0');
}

int validate_float_number(const char* str) {
    int len_str = strlen(str);
    if (len_str == 0) {
        return 0;
    }
    int num = 0;
    int was_sep = 0;
    for (int i = 0; i < len_str; ++i) {
        if (str[i] == '-') {
            if (i == 0) {
                continue;
            } else {
                return 0;
            }
        }
        if (str[i] == '.' || str[i] == ','){
            if (was_sep) {
                return 0;
            } 
            was_sep = 1;
        } else if ((('0' > str[i] || str[i] > '9') && str[i] != '.' && str[i] != ',' ) || is_separator(str[i])) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char* argv[]) {
    int shm = shm_open(SHM_NAME, O_RDWR, 0666);

    sem_t* sem_child = sem_open(SEM_CHILD, O_EXCL);
    if (sem_child == SEM_FAILED) {
        char* msg = "fail to open semaphore\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(-1);
    }

    sem_t* sem_parent = sem_open(SEM_PARENT, O_EXCL);
    if (sem_parent == SEM_FAILED) {
        char* msg = "fail to open semaphore\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(-1);
    }

    if (argc != 2) {
        const char msg[] = "Usage: ./child <filename>\n";
		write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    int file = open(argv[1], O_RDONLY);

    if (file == -1) {
        const char msg[] = "ERROR: wrong file\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    dup2(file, STDIN_FILENO);
    close(file);


    char buffer[BUFSIZE];
    char msg[BUFSIZE];
    char ch[BUFSIZE];
    int index = 0;
    float current_number = 0;
    float first_number = 0;
    int is_first_number = 1;
    int n = 0;

    float* res = (float*)mmap(0, sizeof(float) * 2, PROT_READ|PROT_WRITE, MAP_SHARED, shm, 0);

    while ((n = read(STDIN_FILENO, ch, sizeof(char))) > 0)
    {
         if (is_separator(*ch)) {
            buffer[index] = '\0';
            if (strlen(buffer) == 0) {
                continue;
            } 
            if (validate_float_number(buffer)) {
                current_number = atof(buffer);
            } else {
                sem_wait(sem_parent);

                res[1] = -1.;
                res[0] = 0.;
                const char msg[] = "ERROR: wrong number\n";
                write(STDOUT_FILENO, msg, sizeof(msg));

                sem_post(sem_child);
                exit(EXIT_FAILURE); 
            }
            if (is_first_number) {
                first_number = current_number;
                is_first_number = 0;
            } else {
                if (current_number != 0) {
                    first_number /= current_number;
                } else {
                    sem_wait(sem_parent);

                    res[0] = 0.;
                    res[1] = -1.;
                    const char msg[] = "ERROR: division by zero\n";
                    write(STDOUT_FILENO, msg, sizeof(msg));

                    sem_post(sem_child);
                    exit(EXIT_FAILURE);
                }
            }
            index = 0;
        } else {
            buffer[index++] = *ch;
        }
    }
    // Обработка последнего числа в файле, если не было разделителя
    if (index > 0) {
        buffer[index] = '\0';
        if (validate_float_number(buffer)) {
            current_number = atof(buffer);
        } else {
            sem_wait(sem_parent);

            res[1] = -1.;
            res[0] = 0.;
            const char msg[] = "ERROR: wrong number\n";
            write(STDOUT_FILENO, msg, sizeof(msg));
            sem_post(sem_child);

            exit(EXIT_FAILURE); 
            
        }
        if (is_first_number) {
            first_number = current_number;
        } else {
            if (current_number != 0) {
                first_number /= current_number;            
            } else {
                sem_wait(sem_parent);

                res[0] = 0.;
                res[1] = -1.;
                const char msg[] = "ERROR: division by zero\n";
                write(STDOUT_FILENO, msg, sizeof(msg));

                sem_post(sem_child);
                exit(EXIT_FAILURE);
            }
        }
    }
    
    sem_wait(sem_parent);
    res[0] = first_number;
    sem_post(sem_child);

    close(STDIN_FILENO);
    munmap(res, sizeof(float) * 2);
    sem_close(sem_child);
    sem_close(sem_parent);

    return 0;
}