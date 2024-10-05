#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

void print_error(const char* msg) {
    write(STDERR_FILENO, msg, sizeof(msg));
    write(STDERR_FILENO, "\n", 1);
}

void print_usage(const char* program_name) {
    write(STDERR_FILENO, "Usage: ", 7);
    write(STDERR_FILENO, program_name, sizeof(program_name));
    write(STDERR_FILENO, " <output_file>\n", 15);
}

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
        print_usage(argv[0]);
        exit(1);
    }

    // Открытие файла для записи
    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        print_error("Cannot open file");
        exit(1);
    }

    // Чтение данных из стандартного ввода
    char buffer[1024];
    ssize_t bytes_read;

    // Обработка данных и запись в файл
    while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        remove_vowels(buffer);  // Удаление гласных
        write(fd, buffer, bytes_read);  // Запись в файл
    }

    // Закрытие файла после записи
    close(fd);

    return 0;
}
