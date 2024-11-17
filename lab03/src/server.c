#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_SIZE 4096  
#define FILENAME_SIZE 4096 

int intToStr(int num, char *str) {
    int i = 0;
    if (num < 0) {
        str[i++] = '-';
        num = -num;
    }
    int start = i;
    do {
        str[i++] = (num % 10) + '0';
        num /= 10;
    } while (num > 0);
    
    
    for (int j = start, end = i - 1; j < end; j++, end--) {
        char temp = str[j];
        str[j] = str[end];
        str[end] = temp;
    }
    str[i] = '\0';
    return i;
}

int main() {
    int shmid;
    char *shared_memory;

    shmid = shmget(IPC_PRIVATE, SHM_SIZE + FILENAME_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        const char msg[] = "ошибка: не удалось создать разделяемую память\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
    }

    shared_memory = (char *)shmat(shmid, NULL, 0);
    if (shared_memory == (char *)(-1)) {
        const char msg[] = "ошибка: не удалось привязать разделяемую память\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
    }

    const char msg[] = "Введите имя файла: ";
	write(STDIN_FILENO, msg, sizeof(msg));    
    read(STDIN_FILENO, shared_memory + SHM_SIZE, FILENAME_SIZE);

    shared_memory[SHM_SIZE + strcspn(shared_memory + SHM_SIZE, "\n")] = 0;

    char msgForShmid[] = "Идентификатор разделяемой памяти: ";
    msgForShmid[strlen(msgForShmid)] = intToStr(shmid, msgForShmid + strlen(msgForShmid));
    int len = strlen(msgForShmid);
	write(STDOUT_FILENO, msgForShmid, len);
    write(STDOUT_FILENO, "\n", 1);

    while (1) {
        const char msg[] = "Введите строку (или нажмите CTRL+D для завершения): ";
        write(STDIN_FILENO, msg, sizeof(msg));
        if (read(STDIN_FILENO, shared_memory, SHM_SIZE) <= 0) {
            break; 
        }        
        shared_memory[strcspn(shared_memory, "\n")] = 0;
   
        if (strlen(shared_memory) == 0) {
            break;
        }
    }
    strcpy(shared_memory, "exit");

    shmdt(shared_memory);
    shmctl(shmid, IPC_RMID, NULL);

    write(STDOUT_FILENO, "Сервер завершен.\n", strlen("Сервер завершен.\n"));
    return 0;
}