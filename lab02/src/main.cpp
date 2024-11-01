#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <string>

using std::string;
using std::vector;
// variant 9 Рассчитать детерминант матрицы (используя определение детерминанта)

inline size_t min(size_t a, size_t b) {
    return a < b ? a : b;
}

inline ssize_t print(int fd, string s) {
    return write(fd, s.c_str(), s.size());
}

typedef struct Args {
    vector<vector<double>> *mat;
    double *det;
    size_t i;
    size_t cnt;
} args_t;

double det(vector<vector<double>> &mat, size_t off);

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
    *(args->det) = det(mat, 0);
    return NULL;
}

double det(vector<vector<double>> &mat, size_t rem) {
    if (mat.size() == 1)
        return mat[0][0];

    vector<double> dets(mat.size());
    vector<args_t *> args(mat.size());

    if (rem > 0) {
        vector<pthread_t> th(min(rem, mat.size()));
        rem -= th.size();
        size_t first = rem % (th.size() - 1);
        size_t rest = rem / (th.size() - 1);
        size_t i = 0;
        while (i < mat.size()) {
            size_t j;
            for (j = 0; j < th.size() && i < mat.size(); j++) {
                args[i] = (args_t *)malloc(sizeof(args_t));
                if (args[i] == NULL) {
                    print(STDOUT_FILENO, "ERROR: buy more ram\n");
                    exit(-1);
                }
                args[i]->mat = &mat;
                args[i]->det = &dets[i];
                args[i]->i = i;
                args[i]->cnt = j == 0 ? first : rest;
                int s = pthread_create(&th[j], NULL, calc, args[i]);
                if (s != 0) {
                    print(STDOUT_FILENO,
                          "ERROR: failed to create new thread\n");
                    exit(-1);
                }
                i++;
            }
            for (size_t k = 0; k < j; k++) {
                pthread_join(th[k], NULL);
            }
        }
    } else {
        for (size_t i = 0; i < mat.size(); i++) {
            args[i] = (args_t *)malloc(sizeof(args_t));
            args[i]->mat = &mat;
            args[i]->det = &dets[i];
            args[i]->i = i;
            args[i]->cnt = 0;
            calc(args[i]);
        }
    }

    double res = 0;
    for (size_t i = 0; i < dets.size(); i++) {
        free(args[i]);
        double mul = i % 2 == 0 ? 1 : -1;
        res += mul * mat[0][i] * dets[i];
    }
    return res;
}

double determinant(vector<vector<double>> &mat, size_t thread_cnt) {
    for (size_t i = 0; i < mat.size(); i++) {
        if (mat[i].size() != mat.size()) {
            print(STDERR_FILENO, "ERROR: matrix must be square\n");
            return 0;
        }
    }
    return det(mat, thread_cnt);
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

    // vector<vector<double>> c = {{0, 1, 2, 4, 5, 6},     {2, 3, 1, 0, -1, -2},
    //                             {12, 321, 1, 2, 41, 1}, {12, 31, 1, 2, 41,
    //                             1},
    //                             {-12, 32, 1, 2, 0, -1}, {1, 0, 0, 0, 1, 2}};

    vector<vector<double>> c(10, vector<double>(10, 1));

    double det = determinant(c, thread_number);
    string res = "|mat| = " + std::to_string(det) + "\n";
    print(STDOUT_FILENO, res);

    return 0;
}
