#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <strsafe.h>

#define DECK_SIZE 52
#define MAX_THREADS 50

typedef struct {
    int trials;
    int matches;
} ThreadData;

CRITICAL_SECTION cs;

void shuffle_deck(int *deck) {
    for (int i = DECK_SIZE - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

int check_for_match(int *deck) {
    return deck[0] == deck[1];
}

void init_deck(int *deck) {
    for (int i = 0; i < DECK_SIZE; i++) {
        deck[i] = i % (DECK_SIZE / 4);
    }
}

void write_to_console(const char *str) {
    DWORD written;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteConsole(hConsole, str, lstrlenA(str), &written, NULL);
}

DWORD WINAPI monte_carlo_thread(LPVOID lpParam) {
    ThreadData *data = (ThreadData*)lpParam;
    int deck[DECK_SIZE];
    int local_matches = 0;

    init_deck(deck);

    for (int i = 0; i < data->trials; i++) {
        shuffle_deck(deck);
        if (check_for_match(deck)) {
            local_matches++;
        }
    }

    EnterCriticalSection(&cs);
    data->matches += local_matches;
    LeaveCriticalSection(&cs);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        write_to_console("Usage: program.exe <number_of_threads> <trials_per_thread>\n");
        return 1;
    }

    int num_threads = atoi(argv[1]);
    int trials_per_thread = atoi(argv[2]);

    if (num_threads > MAX_THREADS) {
        write_to_console("Too many threads, limiting to MAX_THREADS\n");
        num_threads = MAX_THREADS;
    }

    if (num_threads <= 0 || trials_per_thread <= 0) {
        write_to_console("Invalid arguments.\n");
        return 1;
    }

    InitializeCriticalSection(&cs);

    HANDLE *threads = (HANDLE*)malloc(num_threads * sizeof(HANDLE));
    ThreadData *thread_data = (ThreadData*)malloc(num_threads * sizeof(ThreadData));
    if (!threads || !thread_data) {
        write_to_console("Memory allocation failed.\n");
        return 1;
    }

    srand((unsigned int)time(NULL));

    int total_matches = 0;

    for (int i = 0; i < num_threads; i++) {
        thread_data[i].trials = trials_per_thread;
        thread_data[i].matches = 0;

        threads[i] = CreateThread(NULL, 0, monte_carlo_thread, &thread_data[i], 0, NULL);

        if (threads[i] == NULL) {
            write_to_console("Error creating thread.\n");
            return 1;
        }
    }

    WaitForMultipleObjects(num_threads, threads, TRUE, INFINITE);

    for (int i = 0; i < num_threads; i++) {
        total_matches += thread_data[i].matches;
        CloseHandle(threads[i]);
    }

    double probability = (double)total_matches / (num_threads * trials_per_thread);

    char buffer[100];
    StringCchPrintfA(buffer, 100, "Probability of matching cards on top: %f\n", probability);
    write_to_console(buffer);

    free(threads);
    free(thread_data);
    DeleteCriticalSection(&cs);

    return 0;
}
