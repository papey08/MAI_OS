#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define MAX_POINTS 100
#define FLOAT_PRECISION 6 

typedef enum StatusCode {
    SUCCESS = 0,
    ERROR_COUNT_ARGS,
    ERROR_COUNT_THREADS
} StatusCode;

typedef struct Point {
    double x, y, z;
} Point;

typedef struct ThreadData {
    Point *points;
    int start, end;
    double max_area;
    Point max_triangle[3];
} ThreadData;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
double global_max_area = 0.0;
Point global_max_triangle[3];

double distance(Point a, Point b) {
    return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z));
}

double triangle_area(Point a, Point b, Point c) {
    double ab = distance(a, b);
    double bc = distance(b, c);
    double ca = distance(c, a);
    double s = (ab + bc + ca) / 2.0;
    return sqrt(s * (s - ab) * (s - bc) * (s - ca));
}

void *find_max_triangle(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    double max_area = 0.0;
    Point max_triangle[3];

    for (int i = data->start; i < data->end - 2; i++) {
        for (int j = i + 1; j < data->end - 1; j++) {
            for (int k = j + 1; k < data->end; k++) {
                double area = triangle_area(data->points[i], data->points[j], data->points[k]);
                if (area > max_area) {
                    max_area = area;
                    max_triangle[0] = data->points[i];
                    max_triangle[1] = data->points[j];
                    max_triangle[2] = data->points[k];
                }
            }
        }
    }

    pthread_mutex_lock(&mutex);
    if (max_area > global_max_area) {
        global_max_area = max_area;
        memcpy(global_max_triangle, max_triangle, sizeof(max_triangle));
    }
    pthread_mutex_unlock(&mutex);

    return NULL;
}

StatusCode print_str(const char *str) {
    write(1, str, strlen(str));

    return SUCCESS;
}

StatusCode print_int(int num) {
    char buf[12];
    char *p = buf + sizeof(buf) - 1;
    *p = '\0';

    int is_negative = num < 0;
    if (is_negative) {
        num = -num;
    }

    do {
        *(--p) = '0' + (num % 10);
        num /= 10;
    } while (num > 0);

    if (is_negative) {
        *(--p) = '-';
    }

    print_str(p);

    return SUCCESS;
}

StatusCode print_double(double num) {
    if (num < 0) {
        print_str("-");
        num = -num;
    }

    int int_part = (int)num;
    double fraction_part = num - int_part;

    print_int(int_part);

    print_str(".");
    for (int i = 0; i < FLOAT_PRECISION; i++) {
        fraction_part *= 10;
        int digit = (int)fraction_part;
        fraction_part -= digit;
        char c = '0' + digit;
        write(1, &c, 1);
    }

    return SUCCESS;
}

StatusCode print_point(Point p) {
    print_str("(");
    print_double(p.x);
    print_str(", ");
    print_double(p.y);
    print_str(", ");
    print_double(p.z);
    print_str(")\n");

    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(2, "Неверное количество аргументов.\n", strlen("Неверное количество аргументов.\n"));
        return ERROR_COUNT_ARGS;
    }

    int max_threads = atoi(argv[1]);
    if (max_threads <= 0) {
        return ERROR_COUNT_THREADS;
    }

    Point points[MAX_POINTS] = {
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}, {2.0, 2.0, 2.0},
        {3.0, 3.0, 3.0}, {1.0, 2.0, 3.0}, {4.0, 4.0, 4.0},
        {-1.0, 2.0, 3.0}, {0.0, 5.0, -1.0}, {3.0, 2.0, 1.0}
    };

    int n_points = sizeof(points) / sizeof(Point);
    int points_per_thread = n_points / max_threads;

    pthread_t threads[max_threads];
    ThreadData thread_data[max_threads];

    for (int i = 0; i < max_threads; i++) {
        thread_data[i].points = points;
        thread_data[i].start = i * points_per_thread;
        thread_data[i].end = (i == max_threads - 1) ? n_points : (i + 1) * points_per_thread;

        pthread_create(&threads[i], NULL, find_max_triangle, &thread_data[i]);
    }

    for (int i = 0; i < max_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    print_str("Максимальная площадь: ");
    print_double(global_max_area);
    print_str("\nТочки треугольника:\n");

    print_str("Координата точки 1: ");
    print_point(global_max_triangle[0]);

    print_str("Координата точки 2: ");
    print_point(global_max_triangle[1]);

    print_str("Координата точки 3: ");
    print_point(global_max_triangle[2]);

    return SUCCESS;
} 