#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_SIZE 5000

// Функция для нахождения последней последовательности из N символов, где каждый больше предыдущего
void find_last_increasing_sequence(char *data, int N, char *result) {
    int length = strlen(data);
    int last_start = -1;

    // Ищем последовательности, где каждый символ больше предыдущего
    for (int i = 0; i <= length - N; i++) {
        int is_increasing = 1;
        for (int j = i + 1; j < i + N; j++) {
            if (data[j] <= data[j - 1]) {
                is_increasing = 0;
                break;
            }
        }
        if (is_increasing) {
            last_start = i;  // запоминаем последний индекс начала последовательности
        }
    }

    if (last_start != -1) {
        strncpy(result, data + last_start, N);
        result[N] = '\0';  // заканчиваем строку
    } else {
        result[0] = '\0';  // если последовательность не найдена
    }
}

// Процесс 1 - чтение из файла и передача через канал
void process_1(int pipe_write_fd, char *input_file) {
    int input_fd = open(input_file, O_RDONLY);
    if (input_fd == -1) {
        perror("Не открыть входной файл(");
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(input_fd, buffer, BUFFER_SIZE);

    if (bytes_read > 0) {
        write(pipe_write_fd, buffer, bytes_read);
    }

    close(input_fd);
    close(pipe_write_fd);
}

// Процесс 2 - поиск последовательности и передача через канал
void process_2(int pipe_read_fd, int pipe_write_fd, int N) {
    char buffer[BUFFER_SIZE];
    char result[BUFFER_SIZE];
    ssize_t bytes_read = read(pipe_read_fd, buffer, BUFFER_SIZE);

    if (bytes_read > 0) {
        find_last_increasing_sequence(buffer, N, result);
        write(pipe_write_fd, result, strlen(result));
    }

    close(pipe_read_fd);
    close(pipe_write_fd);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Формат вызова: %s <input_file> <output_file> <N>\n", argv[0]);
        return 1;
    }

    char *input_file = argv[1];
    char *output_file = argv[2];
    int N = atoi(argv[3]);

    if (N <= 0) {
        fprintf(stderr, "N д.б больше 1!.\n");
        return 1;
    }

    // Создание неименованных каналов
    int pipe1[2], pipe2[2];
    if (pipe(pipe1) == -1) {
        perror("Ошибка при создании pipe1");
        return 1;
    }
    if (pipe(pipe2) == -1) {
        perror("Ошибка при создании pipe2");
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        // Процесс 1
        close(pipe1[0]);  // Закрытие неиспользуемого конца
        close(pipe2[0]);  // Закрытие неиспользуемого конца
        close(pipe2[1]);  // Закрытие неиспользуемого конца
        process_1(pipe1[1], input_file);  // Передача данных через канал
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        // Процесс 2
        close(pipe1[1]);  // Закрытие неиспользуемого конца
        close(pipe2[0]);  // Закрытие неиспользуемого конца
        process_2(pipe1[0], pipe2[1], N);  // Обработка данных и передача
        exit(0);
    }

    // Процесс 1 - после того, как процесс 2 завершит обработку
    close(pipe1[0]);  // Закрытие неиспользуемого конца
    close(pipe1[1]);  // Закрытие неиспользуемого конца
    close(pipe2[1]);  // Закрытие неиспользуемого конца

    int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd == -1) {
        perror("Не открыть выходной файл(");
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(pipe2[0], buffer, BUFFER_SIZE);

    if (bytes_read > 0) {
        write(output_fd, buffer, bytes_read);
    }

    close(output_fd);
    close(pipe2[0]);

    // Ожидание завершения всех процессов
    wait(NULL);
    wait(NULL);

    return 0;
}
