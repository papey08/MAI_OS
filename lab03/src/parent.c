#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include <sys/types.h>  

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
    char buffer[BUFFER_SIZE];
    int count = 1;
    char filename1[BUFFER_SIZE], filename2[BUFFER_SIZE];

    int shmid1 = shmget(IPC_PRIVATE, BUFFER_SIZE, IPC_CREAT | 0666);
    if (shmid1 == -1) {
        write(2, "shmget failed for child 1", strlen("shmget failed for child 1"));
        exit(EXIT_FAILURE);
    }
    int shmid2 = shmget(IPC_PRIVATE, BUFFER_SIZE, IPC_CREAT | 0666);
    if (shmid2 == -1) {
        write(2, "shmget failed for child 2", strlen("shmget failed for child 2"));
        exit(EXIT_FAILURE);
    }

    char *shared_memory1 = (char *)shmat(shmid1, NULL, 0);
    if (shared_memory1 == (char *)-1) {
        write(2, "shmat failed for child 1", strlen("shmat failed for child 1"));
        exit(EXIT_FAILURE);
    }

    char *shared_memory2 = (char *)shmat(shmid2, NULL, 0);
    if (shared_memory2 == (char *)-1) {
        write(2, "shmat failed for child 2", strlen("shmat failed for child 2"));
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
        write(2, "Error creating child 1", strlen("Error creating child 1"));
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        int fd1 = open(filename1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd1 == -1) {
            write(2, "Error opening file for child 1", strlen("Error opening file for child 1"));
            exit(EXIT_FAILURE);
        }

        while (1) {
            if (strlen(shared_memory1) > 0) {
                reverse_string(shared_memory1);
                write(fd1, shared_memory1, strlen(shared_memory1));
                write(fd1, "\n", 1);
                memset(shared_memory1, 0, BUFFER_SIZE);  
            }
            usleep(100000);
        }

        close(fd1);
        exit(EXIT_SUCCESS);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        write(2, "Error creating child 2", strlen("Error creating child 2"));
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        int fd2 = open(filename2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd2 == -1) {
            write(2, "Error opening file for child 2", strlen("Error opening file for child 2"));
            exit(EXIT_FAILURE);
        }

        while (1) {
            if (strlen(shared_memory2) > 0) {
                reverse_string(shared_memory2);
                write(fd2, shared_memory2, strlen(shared_memory2));
                write(fd2, "\n", 1);
                memset(shared_memory2, 0, BUFFER_SIZE); 
            }
            usleep(100000); 
        }

        close(fd2);
        exit(EXIT_SUCCESS);
    }

    while (1) {
        write(1, "Enter the line: ", 16);
        ssize_t bytes_read = read(0, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            break;
        }
        buffer[bytes_read - 1] = '\0'; 

        if (strlen(buffer) == 0) {
            break;
        }

        if (count % 2 == 1) {
            strncpy(shared_memory1, buffer, BUFFER_SIZE);  
        } else {
            strncpy(shared_memory2, buffer, BUFFER_SIZE); 
        }

        if (count % 2 == 1) {
            write(1, "Sending to child1\n", 18);
        } else {
            write(1, "Sending to child2\n", 18);
        }

        count++;
        usleep(100000); 
    }

    shmdt(shared_memory1);
    shmdt(shared_memory2);
    shmctl(shmid1, IPC_RMID, NULL);
    shmctl(shmid2, IPC_RMID, NULL);

    return 0;
}
