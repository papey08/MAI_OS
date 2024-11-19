#include <stdint.h> 
#include <stdbool.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <fcntl.h>
#include <ctype.h> // Для isupper()
#include <sys/mman.h>
#include <sys/stat.h> // Для shm_open

// Проверить, что строка начинается с заглавной буквы

int main(int argc, char **argv) {
	char buf[4096];
	ssize_t bytes;

	pid_t pid = getpid();

	// Открываем разделяемую память
	int shm_fd = shm_open("/my_shared_memory", O_RDONLY, 0666);
	if (shm_fd == -1) {
		const char msg[] = "error: failed to open shared memory\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	void *shared_memory = mmap(0, 4096, PROT_READ, MAP_SHARED, shm_fd, 0);
	if (shared_memory == MAP_FAILED) {
		const char msg[] = "error: failed to map shared memory\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0600);
	if (file == -1) {
		const char msg[] = "error: failed to open requested file\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	{
		char msg[128];
		int32_t len = snprintf(msg, sizeof(msg) - 1, 
        "%d: Start typing lines of text.Press 'Ctrl-D' or 'Enter' with no input to exit\n",
        pid);
		write(STDOUT_FILENO, msg, len);
	}

	while (bytes = read(STDIN_FILENO, buf, sizeof(buf))) {
		if (bytes < 0) {
			const char msg[] = "error: failed to read from stdin\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		} else if (buf[0] == '\n') {
			break;
		}

		if (!isupper(buf[0])) {
			const char msg[] = "error: string does not start with uppercase letter\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			continue;
		}

		{
			char msg[32];
			int32_t len = snprintf(msg, sizeof(msg) - 1, "");

			int32_t written = write(file, msg, len);
			if (written != len) {
				const char msg[] = "error: failed to write to file\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			}
		}

		{
			buf[bytes - 1] = '\0';

			int32_t written = write(file, buf, bytes);
			if (written != bytes) {
				const char msg[] = "error: failed to write to file\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			}
		}
	}

    if (bytes == 0) {
		const char msg[] = "\nEnd of input detected (Ctrl+D)\n";
		write(STDOUT_FILENO, msg, sizeof(msg));
	}

	const char term = '\0';
	write(file, &term, sizeof(term));

	close(file);
	munmap(shared_memory, 4096);
	close(shm_fd);
}
