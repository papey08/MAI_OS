#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#define SHM_SIZE 1024

void handle_error(const char *msg) {
    const char *error_message = ": Ошибка\n";
    write(2, msg, strlen(msg));
    write(2, error_message, strlen(error_message));
    _exit(EXIT_FAILURE);
}

void write_message(const char *msg) {
    write(1, msg, strlen(msg));
}

void read_message(char *buffer, size_t size) {
    ssize_t bytes_read = read(0, buffer, size - 1);
    if (bytes_read <= 0) handle_error("Ошибка чтения");
    buffer[bytes_read - 1] = '\0';
}

int main() {
    int fd = open("shared_memory", O_CREAT | O_RDWR, 0666);
    if (fd == -1) handle_error("Ошибка создания файла");
    close(fd);

    key_t key = ftok("shared_memory", 65);
    if (key == -1) handle_error("ftok");

    int shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) handle_error("shmget");

    char *shared_memory = (char *)shmat(shmid, NULL, 0);
    if (shared_memory == (char *)-1) handle_error("shmat");

    write_message("Введите строку (или пустую строку для выхода): ");
    char input_buffer[SHM_SIZE];

    while (1) {
        read_message(input_buffer, SHM_SIZE);

        if (strcmp(input_buffer, "") == 0) break;

        strcpy(shared_memory, input_buffer);

        pid_t pid1 = fork();
        if (pid1 == -1) handle_error("fork");

        if (pid1 == 0) {
            execl("./child1", "./child1", NULL);
            handle_error("execl (child1)");
        }

        wait(NULL);

        pid_t pid2 = fork();
        if (pid2 == -1) handle_error("fork");

        if (pid2 == 0) {
            execl("./child2", "./child2", NULL);
            handle_error("execl (child2)");
        }

        wait(NULL);

        write_message("Результат обработки: ");
        write_message(shared_memory);
        write_message("\nВведите строку (или пустую строку для выхода): ");
    }

    if (shmdt(shared_memory) == -1) handle_error("shmdt");
    if (shmctl(shmid, IPC_RMID, NULL) == -1) handle_error("shmctl");

    return 0;
}
