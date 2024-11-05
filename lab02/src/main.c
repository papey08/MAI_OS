#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>

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

void* calc_score(void* arg) {
    int rounds = *(int*)arg;
    int* score = malloc(sizeof(int) * 2);
    for (int i = 0; i < rounds; i++)
    {
        score[0] += rand() % 6 + 1;
        score[0] += rand() % 6 + 1;

        score[1] += rand() % 6 + 1;
        score[1] += rand() % 6 + 1;

    }
    *(int*)arg = score[0];
    free(arg);
    return score;
}

// ввод (5 цифорок):
// количество бросков двух костей, какой сейчас тур, сколько очков суммарно у каждого из игроков и
// количество экспериментов, которые должна произвести программа
int main(int argc, char* argv[]) {
    srand(time(NULL));
    if (argc != 6) {
        printf("USAGE: ./a.out <total rounds> <current tour> <first_score> <second_score> <experiments>\n");
        return 1;
    }

    if (validate_nums(argc, argv)) {
        printf("ERROR: all input numbers must be integer and positive\n");
        return 2;
    }

    int k = atoi(argv[1]);
    int tour = atoi(argv[2]);
    int first_score = atoi(argv[3]);
    int second_score = atoi(argv[4]);
    int experiments = atoi(argv[5]);

    if (k < tour) {
        printf("ERROR: total rounds must be greater than current tour\n");
        return 3;
    }

    // если игра уже окончена, то у одного шанс на победу 100%, а у второго 0%
    if (k == tour) {
        if (first_score > second_score) {
            printf("First player win with 100%% probability\n");
            printf("Second player win with 0%% probability\n");
        } else if (first_score < second_score) {
            printf("First player win with 0%% probability\n");
            printf("Second player win with 100%% probability\n");
        } else {
            printf("First player win with 100%% probability\n");
            printf("Second player win with 100%% probability\n");
        }
        return 0;
    }

    pthread_t experiments_threads[experiments];

    for (int i = 0; i < experiments; ++i) {
        int* rounds = malloc(sizeof(int));
        *rounds = k - tour;
        if (pthread_create(&experiments_threads[i], NULL, &calc_score, rounds)) {
            printf("ERROR: thread cannot be created\n");
            return 4;
        }
    }

    float first_prob = 0.;
    float second_prob = 0.;
    float exp = (float)experiments;

    for (int i = 0; i < experiments; ++i) {
        int* scores;
        if (pthread_join(experiments_threads[i], (void**)&scores)) {
            printf("ERROR: thread cannot be joined\n");
            return 5;
        }
        if ((scores[0] + first_score) > (scores[1] + second_score)) {
            first_prob += 1;
        } else if ((scores[0] + first_score) < (scores[1] + second_score)) {
            second_prob += 1;
        } else {
            exp--;
        }
        // вывести очки по итогу одного эксперимента
        // printf("%d %d\n", scores[0], scores[1]);
    }
    first_prob = first_prob / (float)exp * 100;
    second_prob =  second_prob / (float)exp * 100;
    printf("First player win with %.2f%% probability\n", first_prob);
    printf("Second player win with %.2f%% probability\n", second_prob);
    return 0;
}