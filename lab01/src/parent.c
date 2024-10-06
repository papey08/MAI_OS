#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

void reverse_string(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = temp;
    }
}

int main() {
    int pipe1[2], pipe2[2];
    char buffer[BUFFER_SIZE];
    int count = 1;
    char filename1[BUFFER_SIZE];
    char filename2[BUFFER_SIZE];

    const char* pipe_error = "Error creating pipe.\n";
    const char* child1_error = "Error creating process 1.\n";
    const char* child11_error = "Error opening file for child1.\n";
    const char* child2_error = "Error creating process 2.\n";
    const char* child22_error = "Error opening file for child2.\n";


    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        write(2, pipe_error, strlen(pipe_error));
        exit(EXIT_FAILURE);
    }

    write(1, "Enter a filename for child1: ", strlen("Enter a filename for child1: "));
    read(0, filename1, BUFFER_SIZE);
    filename1[strcspn(filename1, "\n")] = '\0'; 

    write(1, "Enter a filename for child2: ", strlen("Enter a filename for child2: "));
    read(0, filename2, BUFFER_SIZE);
    filename2[strcspn(filename2, "\n")] = '\0'; 

    
    pid_t pid1 = fork();
    if (pid1 == -1) {
        write(2, child1_error, strlen(child1_error));
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        int fd1 = open(filename1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd1 == -1) {
            write(2, child11_error, strlen(child11_error));
            exit(EXIT_FAILURE);
        }
        
        close(pipe1[1]); 
        dup2(pipe1[0], 0); 
        dup2(fd1, 1); 
        close(pipe1[0]);
        close(fd1);

        execl("./child1", "child1", NULL); 
        write(2, "exec error for child1.\n", strlen("exec error for child1.\n"));
        exit(EXIT_FAILURE);
    }

    
    pid_t pid2 = fork();
    if (pid2 == -1) {
        write(2, child2_error, strlen(child2_error));
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        int fd2 = open(filename2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd2 == -1) {
            write(2, child22_error, strlen(child22_error));
            exit(EXIT_FAILURE);
        }

        close(pipe2[1]); 
        dup2(pipe2[0], 0); 
        dup2(fd2, 1); 
        close(pipe2[0]);
        close(fd2);

        execl("./child2", "child2", NULL); 
        write(2, "exec error for child2.\n", strlen("exec error for child2.\n"));
        exit(EXIT_FAILURE);
    }

    
    close(pipe1[0]); 
    close(pipe2[0]);


    while (1) {
        write(1, "Enter the line: ", 16);
        ssize_t bytes_read = read(0, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            exit(EXIT_SUCCESS);
        }
        buffer[bytes_read - 1] = '\0'; 

        if (strlen(buffer) == 0) {
            break;
        }

        if (count % 2 == 0) {
            write(pipe2[1], buffer, bytes_read);
        } else {
            write(pipe1[1], buffer, bytes_read);
        }
        count++;
    }

  
    close(pipe1[1]);
    close(pipe2[1]);

   


    return 0;
}







