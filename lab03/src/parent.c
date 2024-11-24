#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 1024
#define SHM_SIZE 4096

int main() {
    pid_t pid;


    int shm_fd = shm_open("/my_shared_memory", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        const char msg[] = "error: failed to open shared memory\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(EXIT_FAILURE);
    }


    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        const char msg[] = "error: failed to truncate shared memory\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(EXIT_FAILURE);
    }


    void *ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        const char msg[] = "error: failed to mmap\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(EXIT_FAILURE);
    }


    char filename[BUF_SIZE];
    const char prompt[] = "Enter filename: ";
    write(STDOUT_FILENO, prompt, strlen(prompt));
    int n = read(STDIN_FILENO, filename, sizeof(filename) - 1);

    if (n > 0) {
        filename[n - 1] = '\0';
    } else {
        filename[0] = '\0';
    }


    pid = fork();
    if (pid == -1) {
        const char msg[] = "error: failed to fork\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        execlp("./child", "child", filename, (char *)NULL);
        const char msg[] = "error: failed to execlp\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);

        const char msg[] = "Read from shared memory:\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        write(STDOUT_FILENO, (char *)ptr, strlen((char *)ptr));

        munmap(ptr, SHM_SIZE);
        close(shm_fd);
        shm_unlink("/my_shared_memory");
    }

    return 0;
}
