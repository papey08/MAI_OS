#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

void handle_error(const char *message) {
    write(STDERR_FILENO, message, strlen(message));
    exit(EXIT_FAILURE);
}

int is_prime(int num) {
    if (num < 2) {
        return 0;
    }
    for (int i = 2; i * i <= num; i++) {
        if (num % i == 0) {
            return 0;
        } 
    }
    return 1;
}

int int_to_str(int num, char *buf, int buf_size) {
    int len = 0;
    int temp = num;

    if (num < 0) {
        if (buf_size > 1) {
            buf[len++] = '-';
            num = -num;
        } else {
            return -1;
        }
    }

    do {
        temp /= 10;
        len++;
    } while (temp > 0);

    if (len >= buf_size) {
        return -1;
    }

    buf[len] = '\0';
    while (num > 0) {
        buf[--len] = (num % 10) + '0';
        num /= 10;
    }

    return strlen(buf);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        handle_error("Ошибка: имя файла не передано\n");
    }

    const char *filename = argv[1];
    int number, signal;

    int file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_fd == -1) {
        handle_error("Ошибка при открытии файла\n");
    }

    while (1) {
        if (read(STDIN_FILENO, &number, sizeof(number)) <= 0) {
            handle_error("Ошибка при чтении из pipe1\n");
        }

        if (number < 0 || is_prime(number)) {
            signal = 1;
            if (write(STDOUT_FILENO, &signal, sizeof(signal)) == -1) {
                handle_error("Ошибка при записи сигнала в pipe2\n");
            }
            break;
        } else {
            char buf[128];
            int len = int_to_str(number, buf, sizeof(buf));
            if (len == -1) {
                handle_error("Ошибка при преобразовании числа в строку\n");
            }

            if (write(file_fd, buf, len) == -1) {
                handle_error("Ошибка при записи в файл\n");
            }

            if (write(file_fd, " ", 1) == -1) {
                handle_error("Ошибка при записи пробела в файл\n");
            }

            signal = 0;
            if (write(STDOUT_FILENO, &signal, sizeof(signal)) == -1) {
                handle_error("Ошибка при записи сигнала в pipe2\n");
            }
        }
    }

    if (close(file_fd) == -1) {
        handle_error("Ошибка при закрытии файла\n");
    }

    exit(0);
}
