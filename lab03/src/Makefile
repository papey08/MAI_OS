# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11

# Имена исходных файлов и исполняемых программ
PARENT_SRC = parent.c
CHILD_SRC = child.c
PARENT_EXEC = parent
CHILD_EXEC = child

# Цель по умолчанию
all: $(PARENT_EXEC) $(CHILD_EXEC)

# Сборка родительского процесса
$(PARENT_EXEC): $(PARENT_SRC)
	$(CC) $(CFLAGS) -o $(PARENT_EXEC) $(PARENT_SRC)

# Сборка дочернего процесса
$(CHILD_EXEC): $(CHILD_SRC)
	$(CC) $(CFLAGS) -o $(CHILD_EXEC) $(CHILD_SRC)

# Очистка всех скомпилированных файлов
clean:
	rm -f $(PARENT_EXEC) $(CHILD_EXEC)

# Пересборка проекта
rebuild: clean all