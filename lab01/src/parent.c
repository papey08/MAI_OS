#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define BUFFER_SIZE 1024

BOOL running = TRUE;
HANDLE file_handle = INVALID_HANDLE_VALUE;
HANDLE pipe1_write = INVALID_HANDLE_VALUE;
HANDLE pipe2_read = INVALID_HANDLE_VALUE;
PROCESS_INFORMATION pi = {0};
char last_input[BUFFER_SIZE] = {0};

void cleanup() {
    if (pi.hProcess != NULL) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hProcess);
        pi.hProcess = NULL;
    }
    if (pi.hThread != NULL) {
        CloseHandle(pi.hThread);
        pi.hThread = NULL;
    }
    if (file_handle != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(file_handle);
        CloseHandle(file_handle);
        file_handle = INVALID_HANDLE_VALUE;
    }
    if (pipe1_write != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(pipe1_write);
        CloseHandle(pipe1_write);
        pipe1_write = INVALID_HANDLE_VALUE;
    }
    if (pipe2_read != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe2_read);
        pipe2_read = INVALID_HANDLE_VALUE;
    }
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    if (fdwCtrlType == CTRL_C_EVENT) {
        DWORD bytes_written;
        const char *msg = "\nПолучен сигнал завершения. Завершение работы...\n";
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), msg, strlen(msg), &bytes_written, NULL);
        running = FALSE;
        const char *exit_signal = "EXIT\n";
        WriteFile(pipe1_write, exit_signal, strlen(exit_signal), NULL, NULL);
        cleanup();
        return TRUE;
    }
    return FALSE;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);

    HANDLE pipe1_read = INVALID_HANDLE_VALUE;
    HANDLE pipe2_write = INVALID_HANDLE_VALUE;
    SECURITY_ATTRIBUTES sa = {0};
    STARTUPINFO si = {0};
    BOOL success;
    char filename[BUFFER_SIZE];
    char input[BUFFER_SIZE];
    char error_message[BUFFER_SIZE];

    if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
        const char *err_msg = "ERROR: Could not set control handler\n";
        DWORD bytes_written;
        WriteConsole(GetStdHandle(STD_ERROR_HANDLE), err_msg, strlen(err_msg), &bytes_written, NULL);
        return 1;
    }

    const char *prompt = "Введите имя файла для записи: ";
    DWORD bytes_written;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), prompt, strlen(prompt), &bytes_written, NULL);

    DWORD bytes_read;
    ReadConsole(GetStdHandle(STD_INPUT_HANDLE), filename, BUFFER_SIZE, &bytes_read, NULL);
    filename[bytes_read - 2] = '\0';

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&pipe1_read, &pipe1_write, &sa, 0)) {
        const char *err_msg = "Ошибка при создании pipe1\n";
        WriteConsole(GetStdHandle(STD_ERROR_HANDLE), err_msg, strlen(err_msg), &bytes_written, NULL);
        return 1;
    }

    if (!CreatePipe(&pipe2_read, &pipe2_write, &sa, 0)) {
        const char *err_msg = "Ошибка при создании pipe2\n";
        WriteConsole(GetStdHandle(STD_ERROR_HANDLE), err_msg, strlen(err_msg), &bytes_written, NULL);
        CloseHandle(pipe1_read);
        CloseHandle(pipe1_write);
        return 1;
    }

    si.cb = sizeof(si);
    si.hStdInput = pipe1_read;
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = pipe2_write;
    si.dwFlags |= STARTF_USESTDHANDLES;

    if (!CreateProcess(NULL, "child.exe", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        const char *err_msg = "Ошибка при создании дочернего процесса\n";
        WriteConsole(GetStdHandle(STD_ERROR_HANDLE), err_msg, strlen(err_msg), &bytes_written, NULL);
        cleanup();
        return 1;
    }

    CloseHandle(pipe1_read);
    CloseHandle(pipe2_write);

    file_handle = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        char err_msg[BUFFER_SIZE];
        wsprintfA(err_msg, "Ошибка при открытии файла: %d\n", GetLastError());
        WriteConsole(GetStdHandle(STD_ERROR_HANDLE), err_msg, strlen(err_msg), &bytes_written, NULL);
        cleanup();
        return 1;
    }

    while (running) {
        const char *input_prompt = "Введите строку (для выхода введите 'exit'): ";
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), input_prompt, strlen(input_prompt), &bytes_written, NULL);

        ReadConsole(GetStdHandle(STD_INPUT_HANDLE), input, BUFFER_SIZE, &bytes_read, NULL);
        input[bytes_read - 2] = '\0';

        if (strcmp(input, "exit") == 0) {
            const char *exit_signal = "EXIT\n";
            WriteFile(pipe1_write, exit_signal, strlen(exit_signal), NULL, NULL);
            break;
        }

        success = WriteFile(pipe1_write, input, strlen(input), &bytes_written, NULL);
        if (!success || bytes_written != strlen(input)) {
            const char *err_msg = "Ошибка при записи в pipe1\n";
            WriteConsole(GetStdHandle(STD_ERROR_HANDLE), err_msg, strlen(err_msg), &bytes_written, NULL);
            break;
        }

        WriteFile(pipe1_write, "\n", 1, &bytes_written, NULL);

        success = ReadFile(pipe2_read, error_message, BUFFER_SIZE - 1, &bytes_read, NULL);
        if (success && bytes_read > 0) {
            error_message[bytes_read] = '\0';
            WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), error_message, bytes_read, &bytes_written, NULL);
        } else {
            strncpy(last_input, input, BUFFER_SIZE);
            last_input[BUFFER_SIZE - 1] = '\0';
            WriteFile(file_handle, input, strlen(input), &bytes_written, NULL);
            WriteFile(file_handle, "\r\n", 2, &bytes_written, NULL);
            FlushFileBuffers(file_handle);
        }
    }

    cleanup();

    return 0;
}