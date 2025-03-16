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
void process_1(int fifo_fd, char *input_file) {
    int input_fd = open(input_file, O_RDONLY);
    if (input_fd == -1) {
        perror("Не открыть входной файл(");
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(input_fd, buffer, BUFFER_SIZE);

    if (bytes_read > 0) {
        write(fifo_fd, buffer, bytes_read);
    }

    close(input_fd);
    close(fifo_fd);
}

// Процесс 2 - поиск последовательности и передача через именованный канал
void process_2(int fifo_in, int fifo_out, int N) {
    char buffer[BUFFER_SIZE];
    char result[BUFFER_SIZE];
    ssize_t bytes_read = read(fifo_in, buffer, BUFFER_SIZE);

    if (bytes_read > 0) {
        find_last_increasing_sequence(buffer, N, result);
        write(fifo_out, result, strlen(result));
    }

    close(fifo_in);
    close(fifo_out);
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

     // Указываем имена FIFO
     const char *fifo1 = "./../tmp/fifo1";
     const char *fifo2 = "./../tmp/fifo2";
 
     // Создание именованных каналов (FIFO)
     if (mkfifo(fifo1, 0666) == -1) {
         perror("Ошибка создания FIFO1");
         return 1;
     }
     if (mkfifo(fifo2, 0666) == -1) {
         perror("Ошибка создания FIFO2");
         return 1;
     }

     pid_t pid1 = fork();
     if (pid1 == 0) {
         // Процесс 1
         int fifo1_fd = open(fifo1, O_WRONLY);
         if (fifo1_fd == -1) {
             perror("Не открыть FIFO1 для записи(");
             exit(1);
         }
         process_1(fifo1_fd, input_file);
         exit(0);
     }
 
     pid_t pid2 = fork();
     if (pid2 == 0) {
         // Процесс 2
         int fifo1_fd = open(fifo1, O_RDONLY);
         if (fifo1_fd == -1) {
             perror("Не открыть FIFO1 для чтения(");
             exit(1);
         }
 
         int fifo2_fd = open(fifo2, O_WRONLY);
         if (fifo2_fd == -1) {
             perror("Не открыть FIFO2 для записи(");
             exit(1);
         }
 
         process_2(fifo1_fd, fifo2_fd, N);
         exit(0);
     }
 
     // Процесс 1 - после того, как процесс 2 завершит обработку
    int fifo2_fd = open(fifo2, O_RDONLY);
    if (fifo2_fd == -1) {
        perror("Не открыть FIFO2 для чтения(");
        exit(1);
    }

    int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd == -1) {
        perror("Не открыть выходной файл(");
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(fifo2_fd, buffer, BUFFER_SIZE);

    if (bytes_read > 0) {
        write(output_fd, buffer, bytes_read);
    }

    close(output_fd);
    close(fifo2_fd);

    // Ожидание завершения всех процессов
    wait(NULL);
    wait(NULL);
    wait(NULL);

    // Удаление FIFO после завершения
    unlink(fifo1);
    unlink(fifo2);

    return 0;
}
