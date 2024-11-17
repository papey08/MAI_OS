#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <fcntl.h> // Для open
#include <sys/types.h> // Для ssize_t
#include <sys/stat.h> // Для mode_t


#define SHM_SIZE 4096
#define FILENAME_SIZE 4096

void write_to_stdout(const char *msg) {
    write(STDOUT_FILENO, msg, strlen(msg));
}

void write_error(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}

int main() {
    int shmid;
    char *shared_memory;
    
    write_to_stdout("Введите идентификатор разделяемой памяти: ");
    char input[10];
    read(STDIN_FILENO, input, sizeof(input));
    shmid = atoi(input);

    shared_memory = (char *)shmat(shmid, NULL, 0);
    if (shared_memory == (char *)(-1)) {
        write_error("shmat: ошибка привязки разделяемой памяти\n");
        exit(1);
    }

    char *filename = shared_memory + SHM_SIZE;

    int file = open(filename, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    if (file < 0) {
        write_error("Ошибка при открытии файла\n");
        exit(1);
    }

    while (1) {

        sleep(1); 

        if (strlen(shared_memory) > 0) {
            
            if (strcmp(shared_memory, "exit") == 0) {
                break; 
            }

            if (isupper(shared_memory[0])) {
                write_to_stdout("Ввод: ");
                write_to_stdout(shared_memory);
                write_to_stdout("\n");

                write(file, shared_memory, strlen(shared_memory));
                write(file, "\n", 1);
                write_to_stdout("Записано в файл: ");
                write_to_stdout(shared_memory);
                write_to_stdout("\n");
            } else {
                write_error("Ошибка: строка должна начинаться с заглавной буквы\n");
            }

            memset(shared_memory, 0, SHM_SIZE);
        }
    }

    close(file);
    shmdt(shared_memory);
    write_to_stdout("Клиент завершен.\n");
    return 0;
}