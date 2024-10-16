#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 1024

int main() {
    HANDLE pipe1_read, pipe2_write;
    BOOL success;
    char buffer[BUFFER_SIZE];
    char line[BUFFER_SIZE];
    int line_pos = 0;

    pipe1_read = GetStdHandle(STD_INPUT_HANDLE);
    pipe2_write = GetStdHandle(STD_ERROR_HANDLE);

    while (1) {
        DWORD bytes_read;
        success = ReadFile(pipe1_read, buffer, BUFFER_SIZE - 1, &bytes_read, NULL);
        if (!success || bytes_read == 0)
            break;

        buffer[bytes_read] = '\0';

        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n' || line_pos == BUFFER_SIZE - 1) {
                line[line_pos] = '\0';
                
                if (strcmp(line, "EXIT") == 0) {
                    goto exit_loop;
                }

                printf("Чтение строки: '%s'\n", line);
                
                if (line_pos > 0 && isupper(line[0])) {
                    printf("%s\n", line);
                } else if (line_pos > 0) {
                    const char *error_message = "Ошибка: строка должна начинаться с заглавной буквы\n";
                    DWORD bytes_written;
                    success = WriteFile(pipe2_write, error_message, strlen(error_message), &bytes_written, NULL);
                    if (!success) {
                        fprintf(stderr, "Ошибка при записи в pipe2\n");
                        return 1;
                    }
                }
                
                line_pos = 0;
            } else {
                line[line_pos++] = buffer[i];
            }
        }
    }

exit_loop:
    CloseHandle(pipe1_read);
    CloseHandle(pipe2_write);

    return 0;
}