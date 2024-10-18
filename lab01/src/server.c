#include <stdint.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

static char CHILD1_PROGRAM_NAME[] = "./child1";
static char CHILD2_PROGRAM_NAME[] = "./child2";

int main(int argc, char **argv) {
    if (argc != 1) {
        char msg[] =  "usage: ./{filename}\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(EXIT_SUCCESS);
    }

    // Get full path to the directory, where program resides
    char progpath[1024];
    {
        // Read full program path, including its name
        ssize_t len = readlink("/proc/self/exe", progpath,
                               sizeof(progpath) - 1);
        if (len == -1) {
            const char msg[] = "error: failed to read full program path\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }

        // Trim the path to first slash from the end
        while (progpath[len] != '/')
            --len;

        progpath[len + 1] = '\0';
    }

    // Open pipe
    int pipe1[2], pipe2[2], pipe3[2];
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1) {
        const char msg[] = "error: failed to create pipe\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    const pid_t child1 = fork();

    switch (child1) {
        case -1: { 
            const char msg[] = "error: failed to spawn new process\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        } break;

        case 0: {
            dup2(pipe1[STDIN_FILENO], STDIN_FILENO);
            dup2(pipe2[STDOUT_FILENO], STDOUT_FILENO);

            close(pipe1[STDOUT_FILENO]);
            close(pipe2[STDIN_FILENO]);
            close(pipe3[STDIN_FILENO]);
            close(pipe3[STDOUT_FILENO]);

            {
                char *const args[] = {CHILD1_PROGRAM_NAME, NULL};

                int32_t status = execv(CHILD1_PROGRAM_NAME, args);

                if (status == -1) {
                    const char msg[] = "error: failed to exec into new exectuable image\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    exit(EXIT_FAILURE);
                }
            }
        } break;
    }

    const pid_t child2 = fork();
    switch (child2) {
        case -1: {
            const char msg[] = "error: failed to spawn new process\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        } break;

        case 0: {

            dup2(pipe2[STDIN_FILENO], STDIN_FILENO);
            dup2(pipe3[STDOUT_FILENO], STDOUT_FILENO);

            close(pipe1[STDIN_FILENO]);
            close(pipe1[STDOUT_FILENO]);

            close(pipe2[STDOUT_FILENO]);
            close(pipe3[STDIN_FILENO]);


            {
                char *const args[] = {CHILD2_PROGRAM_NAME, NULL};

                int32_t status = execv(CHILD2_PROGRAM_NAME, args);

                if (status == -1) {
                    const char msg[] = "error: failed to exec into new exectuable image\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    exit(EXIT_FAILURE);
                }
            }
        } break;
    }
    // closing useless
    close(pipe1[0]);
    close(pipe2[0]);
    close(pipe3[1]);

    ssize_t bytes;
    char buf[1024];

    char msg_of_hint[] = "Enter your string or (Enter / CTRL + D) for stop: \n";
    int len_of_msg_of_hint = strlen(msg_of_hint);
    write(STDOUT_FILENO, msg_of_hint, len_of_msg_of_hint);
    while (bytes = read(STDIN_FILENO, buf, sizeof(buf))) {
        if (bytes < 0) {
			const char msg[] = "error: failed to read from stdin\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		} else if (buf[0] == '\n') {
			break;
		}
        buf[bytes] = '\0';
        // Write into pipe1 for child1 input
        write(pipe1[1], buf, strlen(buf));

        // read from pipe3
        char result[1024];
        ssize_t bytes_read = read(pipe3[0], result, sizeof(result) - 1);
        if (bytes_read > 0) {
            result[bytes_read] = '\0';
            char msg[] = "Processed result: ";
            write(STDOUT_FILENO, msg, strlen(msg));
            write(STDOUT_FILENO, result, bytes_read - 1);
            write(STDOUT_FILENO, "\n\n", 2);
            write(STDOUT_FILENO, msg_of_hint, len_of_msg_of_hint);
        }
        
    }
    close(pipe1[1]);
    close(pipe3[0]);
    close(pipe2[1]);

    wait(NULL);
    wait(NULL);
    return 0;
}