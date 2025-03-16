#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_SIZE 5000

// ПРОГРАММА 1 - ЧТЕНИЕ И ПЕРЕДАЧА ДАННЫХ В ПРОЦЕСС 2 ЧЕРЕЗ FIFO

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
     if (mkfifo(fifo1, 0666) == -1) {
         perror("Ошибка создания FIFO1");
         return 1;
     }
     
     // Открытие FIFO для записи
    int fifo1_fd = open(fifo1, O_WRONLY);
    if (fifo1_fd == -1) {
        perror("Не открыть FIFO1 для записи(");
        return 1;
    }

    process_1(fifo1_fd, input_file);  // Чтение из файла и передача через FIFO

    // Ожидание завершения работы процесса 2
    wait(NULL);

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

    // Удаление FIFO после завершения
    unlink(fifo1);
    unlink(fifo2);

    return 0;
}
