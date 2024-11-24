#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define BUF_SIZE 1024
#define SHM_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 2) {
        const char msg[] = "Usage: ./child <filename>\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(EXIT_FAILURE);
    }


    int filefd = open(argv[1], O_RDONLY);
    if (filefd == -1) {
        const char msg[] = "error: failed to open file\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(EXIT_FAILURE);
    }


    int shm_fd = shm_open("/my_shared_memory", O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        close(filefd);
        exit(EXIT_FAILURE);
    }


    void *ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        close(filefd);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }


    dup2(filefd, STDIN_FILENO);
    close(filefd);


    char buffer[BUF_SIZE];
    size_t bytes_read;
    char num_str[BUF_SIZE];
    int num_index = 0;
    int in_number = 0;
    double sum = 0;


    while ((bytes_read = read(STDIN_FILENO, buffer, BUF_SIZE)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            if (isdigit(buffer[i]) || buffer[i] == '.' || (buffer[i] == '-' && !in_number)) {
                if (num_index < BUF_SIZE - 1) {
                    num_str[num_index++] = buffer[i];
                    in_number = 1;
                } else {
                    const char msg[] = "error: number too long\n";
                    write(STDERR_FILENO, msg, strlen(msg));
                    munmap(ptr, SHM_SIZE);
                    close(shm_fd);
                    exit(EXIT_FAILURE);
                }
            } else if (in_number) {
                num_str[num_index] = '\0';
                double number = atof(num_str);
                sum += number;
                num_index = 0;
                in_number = 0;
            }
        }
    }

    if (bytes_read == -1) {
        perror("read");
        munmap(ptr, SHM_SIZE);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }


    if (in_number) {
        num_str[num_index] = '\0';
        double number = atof(num_str);
        sum += number;
    }


    size_t n = snprintf(ptr, SHM_SIZE, "Sum: %f\n", sum);
    if (n >= SHM_SIZE) {
        const char msg[] = "error: output too large for shared memory\n";
        write(STDERR_FILENO, msg, strlen(msg));
        munmap(ptr, SHM_SIZE);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    
    munmap(ptr, SHM_SIZE);  
    close(shm_fd);
    close(filefd);

    return 0;
}
