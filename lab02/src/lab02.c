#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#define MAX_POINTS 100
#define FLOAT_PRECISION 6 

typedef enum StatusCode {
    SUCCESS = 0,
    ERROR_ARGS_COUNT
} StatusCode;

typedef struct Point {
    double x, y, z;
} Point;

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

StatusCode find_max_triangle(Point *points, int n_points, int start, int end, int pipe_fd) {
    double max_area = 0.0;
    Point max_triangle[3];

    for (int i = start; i < end - 2; i++) {
        for (int j = i + 1; j < n_points - 1; j++) {
            for (int k = j + 1; k < n_points; k++) {
                double area = triangle_area(points[i], points[j], points[k]);
                if (area > max_area) {
                    max_area = area;
                    max_triangle[0] = points[i];
                    max_triangle[1] = points[j];
                    max_triangle[2] = points[k];
                }
            }
        }
    }

    write(pipe_fd, &max_area, sizeof(double));
    write(pipe_fd, &max_triangle, sizeof(max_triangle));
    
    return SUCCESS;
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
    if (is_negative) num = -num;

    do {
        *(--p) = '0' + (num % 10);
        num /= 10;
    } while (num > 0);

    if (is_negative) *(--p) = '-';
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
        return ERROR_ARGS_COUNT;
    }

    int max_processes = atoi(argv[1]);
    if (max_processes <= 0) {
        
        return 1;  
    }

    Point points[MAX_POINTS] = {
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}, {2.0, 2.0, 2.0},
        {3.0, 3.0, 3.0}, {1.0, 2.0, 3.0}, {4.0, 4.0, 4.0},
        {-1.0, 2.0, 3.0}, {0.0, 5.0, -1.0}, {3.0, 2.0, 1.0}
    };

    int n_points = sizeof(points) / sizeof(Point);
    int points_per_process = n_points / max_processes;
    
    pid_t pid;
    int pipe_fds[2];
    pipe(pipe_fds);

    for (int i = 0; i < max_processes; i++) {
        int start = i * points_per_process;
        int end = (i == max_processes - 1) ? n_points : (i + 1) * points_per_process;

        pid = fork();
        if (pid == 0) { 
            close(pipe_fds[0]); 
            find_max_triangle(points, n_points, start, end, pipe_fds[1]);
            close(pipe_fds[1]); 
            exit(0);
        }
    }

    for (int i = 0; i < max_processes; i++) {
        wait(NULL);
    }

    close(pipe_fds[1]); 

    double max_area = 0.0;
    Point max_triangle[3];
    for (int i = 0; i < max_processes; i++) {
        double area;
        Point triangle[3];
        read(pipe_fds[0], &area, sizeof(double));
        read(pipe_fds[0], &triangle, sizeof(triangle));

        if (area > max_area) {
            max_area = area;
            memcpy(max_triangle, triangle, sizeof(triangle));
        }
    }

    print_str("Максимальная площадь: ");
    print_double(max_area);
    print_str("\nТочки треугольника:\n");

    print_str("Координата точки 1: ");
    print_point(max_triangle[0]);

    print_str("Координата точки 2: ");
    print_point(max_triangle[1]);

    print_str("Координата точки 3: ");
    print_point(max_triangle[2]);

    return SUCCESS;
}
