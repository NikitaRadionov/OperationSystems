#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define BUFFER_SIZE 5
#define MSG_KEY1 1234  // Ключ для очереди сообщений для передачи данных
#define MSG_KEY2 5678  // Ключ для очереди сообщений для получения результата

struct message {
    long msg_type;
    char data[BUFFER_SIZE];
};

void process_1(int msgid_send, char *input_file) {
    int input_fd = open(input_file, O_RDONLY);
    if (input_fd == -1) {
        perror("Не открыть входной файл(");
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Читаем файл и отправляем данные порциями
    while ((bytes_read = read(input_fd, buffer, BUFFER_SIZE)) > 0) {
        struct message msg;
        msg.msg_type = 1;  // Тип сообщения для второго процесса
        memcpy(msg.data, buffer, bytes_read);
        if (msgsnd(msgid_send, &msg, bytes_read, 0) == -1) {
            perror("Ошибка отправки сообщения");
            exit(1);
        }
        printf("Отправлено в очередь: %.*s\n", (int)bytes_read, msg.data);  // Отладочный вывод
    }

    close(input_fd);
}

void write_to_file(char *output_file, char *data, ssize_t bytes) {
    int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd == -1) {
        perror("Не открыть выходной файл(");
        exit(1);
    }
    write(output_fd, data, bytes);
    close(output_fd);
    printf("Записано в файл: %.*s\n", (int)bytes, data);  // Отладочный вывод
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
        fprintf(stderr, "N должно быть больше 1!.\n");
        return 1;
    }

    // Получаем доступ к очередям сообщений
    int msgid_send = msgget(MSG_KEY1, 0666 | IPC_CREAT);
    if (msgid_send == -1) {
        perror("Не удалось создать очередь сообщений для отправки");
        exit(1);
    }

    int msgid_receive = msgget(MSG_KEY2, 0666 | IPC_CREAT);
    if (msgid_receive == -1) {
        perror("Не удалось создать очередь сообщений для получения");
        exit(1);
    }

    // Процесс 1 - Чтение из файла и передача через очередь сообщений
    process_1(msgid_send, input_file);

    // Запускаем программу 2 для обработки данных
    pid_t pid = fork();
    if (pid == 0) {
        // Дочерний процесс (процесс 2) вызывает программу 2
        execl("./executable2", "./executable2", input_file, output_file, argv[3], NULL);
        perror("Ошибка при вызове execl");
        exit(1);
    } else if (pid > 0) {
        // Родительский процесс получает обработанные данные и записывает в файл
        struct message msg;
        ssize_t bytes_received;
        while ((bytes_received = msgrcv(msgid_receive, &msg, BUFFER_SIZE, 0, 0)) > 0) {
            printf("Получено сообщение: %.*s\n", (int)bytes_received, msg.data);  // Отладочный вывод
            write_to_file(output_file, msg.data, bytes_received);
        }

        // Ожидаем завершения дочернего процесса
        waitpid(pid, NULL, 0);  // Дожидаемся завершения дочернего процесса

        // Удаление очередей сообщений
        msgctl(msgid_send, IPC_RMID, NULL);
        msgctl(msgid_receive, IPC_RMID, NULL);
    } else {
        perror("Ошибка при вызове fork");
        exit(1);
    }

    return 0;
}
