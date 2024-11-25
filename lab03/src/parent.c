#include <windows.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define SHARED_MEMORY_NAME "SharedMemoryExample"
#define EVENT_WRITE_NAME "WriteEvent"
#define EVENT_READ_NAME "ReadEvent"

HANDLE hMapFile;
LPVOID lpBuffer;
HANDLE hWriteEvent, hReadEvent;
HANDLE hFile;

void cleanup() {
    if (lpBuffer != NULL) {
        UnmapViewOfFile(lpBuffer);
        lpBuffer = NULL;
    }
    if (hMapFile != NULL) {
        CloseHandle(hMapFile);
        hMapFile = NULL;
    }
    if (hWriteEvent != NULL) {
        CloseHandle(hWriteEvent);
        hWriteEvent = NULL;
    }
    if (hReadEvent != NULL) {
        CloseHandle(hReadEvent);
        hReadEvent = NULL;
    }
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }
}

int main() {
    char input[BUFFER_SIZE];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD bytes_written;

    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "Enter the file name to write the correct lines: ", 50, NULL, NULL);
    DWORD bytes_read;
    ReadConsole(GetStdHandle(STD_INPUT_HANDLE), input, BUFFER_SIZE, &bytes_read, NULL);
    input[bytes_read - 2] = '\0';

    hFile = CreateFile(input, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        WriteConsole(GetStdHandle(STD_ERROR_HANDLE), "Error when opening a file for recording\r\n", 42, NULL, NULL);
        return 1;
    }

    hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUFFER_SIZE, SHARED_MEMORY_NAME);
    if (hMapFile == NULL) {
        cleanup();
        return 1;
    }

    lpBuffer = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, BUFFER_SIZE);
    if (lpBuffer == NULL) {
        cleanup();
        return 1;
    }

    hWriteEvent = CreateEvent(NULL, FALSE, FALSE, EVENT_WRITE_NAME);
    hReadEvent = CreateEvent(NULL, FALSE, FALSE, EVENT_READ_NAME);
    if (hWriteEvent == NULL || hReadEvent == NULL) {
        cleanup();
        return 1;
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(NULL, "child.exe", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        WriteConsole(GetStdHandle(STD_ERROR_HANDLE), "Error when starting a child process\r\n", 44, NULL, NULL);
        cleanup();
        return 1;
    }

    while (1) {
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "Enter the line: ", 17, NULL, NULL);
        ReadConsole(GetStdHandle(STD_INPUT_HANDLE), input, BUFFER_SIZE, &bytes_read, NULL);
        input[bytes_read - 2] = '\0';

        if (strcmp(input, "exit") == 0) {
            strcpy((char*)lpBuffer, "EXIT");
            SetEvent(hWriteEvent);
            break;
        }

        strcpy((char*)lpBuffer, input);

        SetEvent(hWriteEvent);

        WaitForSingleObject(hReadEvent, INFINITE);

        if (strncmp((char*)lpBuffer, "ERROR", 5) == 0) {
            WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), (char*)lpBuffer, strlen((char*)lpBuffer), NULL, NULL);
        } else {
            WriteFile(hFile, (char*)lpBuffer, strlen((char*)lpBuffer), &bytes_written, NULL);
            WriteFile(hFile, "\r\n", 2, &bytes_written, NULL);
        }
    }

    cleanup();
    return 0;
}
