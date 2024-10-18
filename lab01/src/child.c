#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 1024

int is_separator(const char c) {
    return (c == ' ') || (c == '\n') || (c == '\t') || (c == '\0');
}

float my_pow(float base, int exp) {
    float result = 1.0;
    int is_negative = 0;

    if (exp < 0) {
        exp = -exp;
        is_negative = 1;
    }

    while (exp > 0) {
        if (exp % 2 == 1) {
            result *= base;
        }
        base *= base;
        exp /= 2;
    }

    if (is_negative) {
        result = 1.0 / result;
    }

    return result;
}

int validate_float_number(const char* str) {
    int len_str = strlen(str);
    if (len_str == 0) {
        return 0;
    }
    int num = 0;
    int was_sep = 0;
    for (int i = 0; i < len_str; ++i) {
        if (str[i] == '-') {
            if (i == 0) {
                continue;
            } else {
                return 0;
            }
        }
        if (str[i] == '.' || str[i] == ','){
            if (was_sep) {
                return 0;
            } 
            was_sep = 1;
        } else if ((('0' > str[i] || str[i] > '9') && str[i] != '.' && str[i] != ',' ) || is_separator(str[i])) {
            return 0;
        }
    }
    return 1;
}

int reverse(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
    return 0;
}

int int_to_str(int num, char* str, int d) {
    int i = 0;
    int is_negative = 1 ? num < 0: 0;
    if (is_negative) {
        num *= -1;
    }
    if (num == 0) {
        str[i++] = '0';
    } else {
        while (num != 0) {
            str[i++] = (num % 10) + '0';
            num /= 10;
        }
    }

    while (i < d) {
        str[i++] = '0';  // добавляем нули, если нужно
    }

    if (is_negative) {
        str[i++] = '-';
    }
    reverse(str, i);
    str[i] = '\0';
    return i;
}

int float_to_str(float n, char* res, int afterpoint) {
    int ipart = (int)n;

    float fpart = n - (float)ipart;
    int i = int_to_str(ipart, res, 0);

    if (afterpoint != 0) {
        res[i] = '.';  
        i++;

        fpart = fpart * my_pow(10, afterpoint);

        int_to_str((int)fpart, res + i, afterpoint);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        const char msg[] = "Usage: ./child <filename>\n";
		write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    int file = open(argv[1], O_RDONLY);

    if (file == -1) {
        const char msg[] = "ERROR: wrong file\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    dup2(file, STDIN_FILENO);
    close(file);


    char buffer[BUFSIZE];
    char msg[BUFSIZE];
    char ch[BUFSIZE];
    int index = 0;
    float current_number = 0;
    float first_number = 0;
    int is_first_number = 1;
    int n = 0;
    while ((n = read(STDIN_FILENO, ch, sizeof(char))) > 0)
    {
         if (is_separator(*ch)) {
            buffer[index] = '\0';
            if (strlen(buffer) == 0) {
                continue;
            } 
            if (validate_float_number(buffer)) {
                current_number = atof(buffer);
            } else {
                const char msg[] = "ERROR: wrong number\n";
                write(STDOUT_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE); 
            }
            if (is_first_number) {
                first_number = current_number;
                is_first_number = 0;
            } else {
                if (current_number != 0) {
                    first_number /= current_number;
                } else {
                    const char msg[] = "ERROR: division by zero\n";
                    write(STDOUT_FILENO, msg, sizeof(msg));
                    exit(EXIT_FAILURE); 
                }
            }
            index = 0;
        } else {
            buffer[index++] = *ch;
        }
    }
    // Обработка последнего числа в файле, если не было разделителя
    if (index > 0) {
        buffer[index] = '\0';
        if (validate_float_number(buffer)) {
            current_number = atof(buffer);
        } else {
            const char msg[] = "ERROR: wrong number\n";
            write(STDOUT_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE); 
        }
        if (is_first_number) {
            first_number = current_number;
        } else {
            if (current_number != 0) {
                first_number /= current_number;            
            } else {
                const char msg[] = "ERROR: division by zero\n";
                write(STDOUT_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
        }
    }
    char str[BUFSIZE];
    float_to_str(first_number, str, 5);
    size_t size_msg = strlen(str);
    str[size_msg] = '\n';
    write(STDOUT_FILENO, str, ++size_msg);
}