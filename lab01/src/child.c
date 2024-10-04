#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int filefd = open(argv[1], O_RDONLY);
    if (filefd == -1) {
        perror("open");
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
                num_str[num_index++] = buffer[i];
                in_number = 1;  
            } else if (in_number) {
                num_str[num_index] = '\0';
                double number = atof(num_str);  
                sum += number;
                num_index = 0;
                in_number = 0;
            }
        }
    }

    if (in_number) {
        num_str[num_index] = '\0';
        double number = atof(num_str);
        sum += number;
    }

    char str[BUF_SIZE];
    size_t n = sprintf(str, "Sum: %f\n", sum);
    write(STDOUT_FILENO, str, n);

    return 0;
}
