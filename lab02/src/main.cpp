#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <vector>

using std::string;
using std::vector;
// variant 9 Рассчитать детерминант матрицы (используя определение детерминанта)

ssize_t print(int fd, string s) {
    return write(fd, s.c_str(), s.size());
}

typedef struct Args {
    vector<vector<double>> *mat;
    double *det;
    size_t i;
} args_t;

double det(vector<vector<double>> &mat);

void *calc(void *ptr) {
    args_t *args = (args_t *)ptr;
    size_t ex = args->i;
    size_t s = args->mat->size();
    vector<vector<double>> mat(s - 1);
    for (size_t i = 0; i < s - 1; i++) {
        mat[i].resize(s - 1);
        for (size_t j = 0; j < ex; j++)
            mat[i][j] = (*(args->mat))[i + 1][j];
        for (size_t j = ex; j < mat.size(); j++)
            mat[i][j] = (*(args->mat))[i + 1][j + 1];
    }
    *(args->det) = det(mat);
    return NULL;
}

double det(vector<vector<double>> &mat) {
    if (mat.size() == 1)
        return mat[0][0];

    vector<double> dets(mat.size());
    vector<pthread_t> threads(mat.size());

    for (size_t i = 0; i < mat.size(); i++) {
        args_t *args = (args_t *)malloc(sizeof(args_t));
        args->mat = &mat;
        args->det = &dets[i];
        args->i = i;
        pthread_create(&threads[i], NULL, calc, args);
    }

    for (size_t i = 0; i < dets.size(); i++) {
        pthread_join(threads[i], NULL);
    }

    double res = 0;
    for (size_t i = 0; i < dets.size(); i++) {
        double mul = i % 2 == 0 ? 1 : -1;
        res += mul * mat[0][i] * dets[i];
    }
    return res;
}

double determinant(vector<vector<double>> &mat) {
    for (size_t i = 0; i < mat.size(); i++) {
        if (mat[i].size() != mat.size()) {
            print(STDERR_FILENO, "ERROR: matrix must be square\n");
            return 0;
        }
    }
    return det(mat);
}

int main(int argc, const char *argw[]) {
    if (argc < 2) {
        print(STDERR_FILENO, "ERROR: number of threads not provided\n");
        return -1;
    }
    char *end;
    size_t thread_number = strtoul(argw[1], &end, 10);
    if (*end != '\0') {
        string message = "ERROR: could not parse \"";
        message += argw[1];
        message += "\" as number\n";
        print(STDERR_FILENO, message);
        return -1;
    }
    if (thread_number == 0) {
        print(STDERR_FILENO, "number of threads must be > 0\n");
        return -1;
    }

    vector<vector<double>> c = {{0, 1, 2, 4, 5, 6},
                                {2, 3, 1, 0, -1, -2},
                                {12, 321, 1, 2, 41, 1},
                                {12, 31, 1, 2, 41, 1},
                                {-12, 32, 1, 2, 0, -1},
                                {1, 0, 0, 0, 1, 2}};
    double det = determinant(c);
    std::cout << "Res: " << det << std::endl;

    return 0;
}
