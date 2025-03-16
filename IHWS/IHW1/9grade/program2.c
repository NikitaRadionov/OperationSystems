#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_SIZE 5

// ПРОГРАММА 2 - ОБРАБОТКА И ПЕРЕДАЧА ДАННЫХ В ПРОЦЕСС 1 ЧЕРЕЗ FIFO

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

// Процесс 2 - поиск последовательности в цикле и передача через именованный канал в цикле
void process_2(int fifo_in, int fifo_out, int N) {
    char buffer[BUFFER_SIZE];
    char result[BUFFER_SIZE];
    char *full_data = NULL;  // Динамически выделяемый буфер для всей строки
    int data_index = 0;
    ssize_t bytes_read;
    size_t buffer_size = 0;

    // Читаем все данные из FIFO и собираем их в динамический буфер
    while ((bytes_read = read(fifo_in, buffer, BUFFER_SIZE)) > 0) {
        // Перераспределяем память для хранения новых данных
        buffer_size += bytes_read;
        full_data = realloc(full_data, buffer_size + 1);  // +1 для нулевого символа

        if (full_data == NULL) {
            perror("Ошибка при выделении памяти(");
            exit(1);
        }

        memcpy(full_data + data_index, buffer, bytes_read);
        data_index += bytes_read;
    }

    // Завершаем строку
    full_data[data_index] = '\0';

    // Ищем последнюю возрастающую последовательность по всей строке
    find_last_increasing_sequence(full_data, N, result);
    write(fifo_out, result, strlen(result));  // Отправляем результат

    // Освобождаем память
    free(full_data);

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

     // Создание именованного канала (FIFO)
    if (mkfifo(fifo2, 0666) == -1) {
        perror("Ошибка создания FIFO2");
        return 1;
    }

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

    process_2(fifo1_fd, fifo2_fd, N); // Обработка данных и передача результата

    // Удаление FIFO после завершения
    unlink(fifo2);

    return 0;
}
