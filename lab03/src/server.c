#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

static char CLIENT_PROGRAM_NAME[] = "client";

int main(int argc, char **argv) {
	if (argc == 1) {
		const char msg[] = "error: no filename provided\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_SUCCESS);
	}

	char progpath[1024];
	{
		ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
		if (len == -1) {
			const char msg[] = "error: failed to read full program path\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}

		while (progpath[len] != '/')
			--len;

		progpath[len] = '\0';
	}

	int shm_fd = shm_open("/my_shared_memory", O_CREAT | O_RDWR, 0666); //O_RDWR - open for read and write
	if (shm_fd == -1) {
		const char msg[] = "error: failed to create shared memory\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	ftruncate(shm_fd, 4096); // size 4096 byte

	void *shared_memory = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); // 0 - OC choose address
	if (shared_memory == MAP_FAILED) {
		const char msg[] = "error: failed to map shared memory\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	const pid_t child = fork();

	switch (child) {
	case -1: { 
		const char msg[] = "error: failed to spawn new process\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	} break;

	case 0: { 
		pid_t pid = getpid(); 

		const char msg[64];
		const int32_t length = snprintf(msg, sizeof(msg), "%d: I'm a child\n", pid);
		write(STDOUT_FILENO, msg, length);

		{
			char path[1024];
			snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT_PROGRAM_NAME);

			char *const args[] = {CLIENT_PROGRAM_NAME, argv[1], NULL};

			int32_t status = execv(path, args);

			if (status == -1) {
				const char msg[] = "error: failed to exec into new executable image\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			}
		}
	} break;

	default: {
		pid_t pid = getpid(); 

		{
			char msg[64];
			const int32_t length = snprintf(msg, sizeof(msg), "%d: I'm a parent, my child has PID %d\n", pid, child);
			write(STDOUT_FILENO, msg, length);
		}

		int child_status;
		wait(&child_status);

		if (child_status != EXIT_SUCCESS) {
			const char msg[] = "error: child exited with error\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(child_status);
		}
	} break;
	}

	// Освобождаем ресурсы
	munmap(shared_memory, 4096);
	shm_unlink("/my_shared_memory");
	close(shm_fd);
}
