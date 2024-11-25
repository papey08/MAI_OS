#include <windows.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 1024
#define SHARED_MEMORY_NAME "SharedMemoryExample"
#define EVENT_WRITE_NAME "WriteEvent"
#define EVENT_READ_NAME "ReadEvent"

int main() {
    HANDLE hMapFile;
    LPVOID lpBuffer;
    HANDLE hWriteEvent, hReadEvent;

    hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHARED_MEMORY_NAME);
    if (hMapFile == NULL) {
        return 1;
    }

    lpBuffer = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, BUFFER_SIZE);
    if (lpBuffer == NULL) {
        CloseHandle(hMapFile);
        return 1;
    }

    hWriteEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, EVENT_WRITE_NAME);
    hReadEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, EVENT_READ_NAME);
    if (hWriteEvent == NULL || hReadEvent == NULL) {
        UnmapViewOfFile(lpBuffer);
        CloseHandle(hMapFile);
        return 1;
    }

    while (1) {
        WaitForSingleObject(hWriteEvent, INFINITE);

        char *line = (char*)lpBuffer;

        if (strcmp(line, "EXIT") == 0) {
            break;
        }

        if (isupper(line[0])) {
            strcpy((char*)lpBuffer, line);
        } else {
            strcpy((char*)lpBuffer, "ERROR: the line must start with a capital letter\r\n");
        }

        SetEvent(hReadEvent);
    }

    UnmapViewOfFile(lpBuffer);
    CloseHandle(hMapFile);
    CloseHandle(hWriteEvent);
    CloseHandle(hReadEvent);

    return 0;
}
