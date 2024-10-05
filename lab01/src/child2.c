#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// Функция для удаления гласных из строки
void remove_vowels(char* str) {
    char* p = str;
    char* q = str;
    while (*p) {
        if (!strchr("AEIOUaeiou", *p)) {
            *q++ = *p;
        }
        p++;
    }
    *q = '\0';
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <output_file>\n", argv[0]);
        exit(1);
    }

    // Открытие файла для записи
    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Cannot open file1");
        exit(1);
    }

    // Чтение данных из стандартного ввода (данные передаются через пайп)
    char buffer[1024];
    ssize_t bytes_read;

    // Обработка данных и запись в файл
    while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        remove_vowels(buffer);  // Удаление гласных
        dprintf(fd, "%s\n", buffer);  // Запись в файл
    }

    // Закрытие файла после записи
    close(fd);

    return 0;
}
