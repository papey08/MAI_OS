#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define MAX_THREADS 12

int validate_nums(int amount, char* nums[]) {
    for (int j = 0; j <= amount; ++j) {
        int len_str = strlen(nums[j]);
        for (int i = 0; i < len_str; ++i) {
            if ('0' > nums[j][i] || nums[j][i] > '9') {
                return 0;
            }
        }
    }
    return 1;
}

void float_to_string(long double value, char *buffer, int len, int precision) {
    for (int i = 0; i < len; ++i) {
        buffer[i] = '\0';
    }
    if (value < 0) {
        *buffer++ = '-';
        value = -value; 
    }
    int integerPart = (int)value;
    float fractionalPart = value - integerPart;

    char *intPtr = buffer;
    if (integerPart == 0) {
        *intPtr++ = '0'; 
    } else {
        char temp[20]; 
        int i = 0;
        while (integerPart > 0) {
            temp[i++] = (integerPart % 10) + '0'; 
            integerPart /= 10; 
        }
        while (i > 0) {
            *intPtr++ = temp[--i];
        }
    }

    *intPtr++ = '.';

    for (int i = 0; i < precision; i++) {
        fractionalPart *= 10; 
        int fractionalDigit = (int)fractionalPart; 
        *intPtr++ = fractionalDigit + '0'; 
        fractionalPart -= fractionalDigit; 
    }

    *intPtr++ = '%';
    *intPtr = '\n';
}

void* calc_score(void* arg) {
    // проблема в том, что использование rand() не позволяет достичь требуемой производительности,
    // поэтому используем свой генератор псевдослучайных чисел

    unsigned long current  = time(NULL);
    const unsigned long a = 1664525;       
    const unsigned long c = 1013904223;    
    const unsigned long m = 4294967296;    


    int rounds = ((int*)arg)[0];
    int exp = ((int*)arg)[1];
    int first_score = ((int*)arg)[2];
    int second_score = ((int*)arg)[3];
    int* score = malloc(sizeof(int) * 2);
    long double* result = malloc(sizeof(long double) * 3);
    for (int j = 0; j < exp; j ++) {
        score[0] = (float)first_score;
        score[1] = (float)second_score;
        for (int i = 0; i < rounds; i++) {
            current = (current * a + c) % m;
            score[0] += current % 6 + 1;
            current = (current * a + c) % m;
            score[0] += current % 6 + 1;

            current = (current * a + c) % m;
            score[1] += current % 6 + 1;
            current = (current * a + c) % m;
            score[1] += current % 6 + 1;
        }

        if(score[0] > score[1]) {
            result[0] += 1.;
            result[2] += 1.;
        } else if(score[0] < score[1]) {
            result[1] += 1.;
            result[2] += 1.;
        }
    }
    free(arg);
    free(score);
    return result;
}

// ввод (6 цифорок):
// количество бросков двух костей, какой сейчас тур, сколько очков суммарно у каждого из игроков,
// количество экспериментов, которые должна произвести программа, количество потоков
int main(int argc, char* argv[]) {
    if (argc != 7) { 
        char msg[] = "USAGE: ./a.out <total rounds> <current tour> <first_score> <second_score> <experiments> <threads>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }

    if (validate_nums(argc, argv)) {
        char msg[] = "ERROR: all input numbers must be integer and positive\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 2;
    }

    int k = atoi(argv[1]);
    int tour = atoi(argv[2]);
    int first_score = atoi(argv[3]);
    int second_score = atoi(argv[4]);
    int experiments = atoi(argv[5]);
    int num_threads = atoi(argv[6]);


    if (k < tour) {
        char msg[] = "ERROR: total rounds must be greater than current tour\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 3;
    } 

    if (num_threads > MAX_THREADS || num_threads < 1) {
        char msg[] = "ERROR: number of threads must be lesser than 12 and greater than 1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 4;
    }

    // если игра уже окончена, то у одного шанс на победу 100%, а у второго 0%
    if (k == tour) {
        if (first_score > second_score) {
            char msg1[] = "First player win with 100%% probability\n";
            char msg2[] = "Second player win with 0%% probability\n";
            write(STDOUT_FILENO, msg1, sizeof(msg1) - 1);
            write(STDOUT_FILENO, msg2, sizeof(msg2) - 1);
        } else if (first_score < second_score) {
            char msg1[] = "First player win with 0%% probability\n";
            char msg2[] = "Second player win with 100%% probability\n";
            write(STDOUT_FILENO, msg1, sizeof(msg1) - 1);
            write(STDOUT_FILENO, msg2, sizeof(msg2) - 1);
        } else {
            char msg1[] = "First player win with 100%% probability\n";
            char msg2[] = "Second player win with 100%% probability\n";
            write(STDOUT_FILENO, msg1, sizeof(msg1) - 1);
            write(STDOUT_FILENO, msg2, sizeof(msg2) - 1);
        }
        return 0;
    }


    pthread_t experiments_threads[num_threads];

    for (int i = 0; i < num_threads; ++i) {
        int* data_for_calc = malloc(sizeof(int) * 4);
        data_for_calc[0] = k - tour;
        data_for_calc[1] = experiments / num_threads + 1;
        data_for_calc[2] = first_score;
        data_for_calc[3] = second_score;
        if (pthread_create(&experiments_threads[i], NULL, calc_score, data_for_calc)) {
            char msg[] = "ERROR: thread cannot be created\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            return 5;
        }
    }

    long double first_prob = 0.;
    long double second_prob = 0.;
    long double exp = 0.;

    for (int i = 0; i < num_threads; ++i) {
        long double* scores;
        if (pthread_join(experiments_threads[i], (void**)&scores)) {
            char msg[] = "ERROR: thread cannot be joined\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            return 6;
        }
        first_prob += scores[0];
        second_prob += scores[1];
        exp += scores[2];
    }

    first_prob = first_prob / exp * 100;
    second_prob = second_prob / exp * 100;

    char num[16];
    float_to_string(first_prob, num, 16, 2);

    char msg1[] = "Probability of the first player winnig - ";
    write(STDOUT_FILENO, msg1, sizeof(msg1) - 1);
    write(STDOUT_FILENO, num, sizeof(num) - 1);

    float_to_string(second_prob, num, 16, 2);
    
    char msg2[] = "Probability of the second player winnig - ";
    write(STDOUT_FILENO, msg2, sizeof(msg2) - 1);
    write(STDOUT_FILENO, num, sizeof(num) - 1);
    return 0;
}