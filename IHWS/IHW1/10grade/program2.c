#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define BUFFER_SIZE 5
#define MSG_KEY1 1234  // Ключ для очереди сообщений для получения данных
#define MSG_KEY2 5678  // Ключ для очереди сообщений для отправки результата

struct message {
    long msg_type;
    char data[BUFFER_SIZE];
};

// Функция для нахождения последней возрастающей последовательности
void find_last_increasing_sequence(char *data, int N, char *result) {
    int length = strlen(data);
    int last_start = -1;

    // Ищем возрастающую последовательность
    for (int i = 0; i <= length - N; i++) {
        int is_increasing = 1;
        for (int j = i + 1; j < i + N; j++) {
            if (data[j] <= data[j - 1]) {
                is_increasing = 0;
                break;
            }
        }
        if (is_increasing) {
            last_start = i;
        }
    }

    if (last_start != -1) {
        strncpy(result, data + last_start, N);
        result[N] = '\0';
    } else {
        result[0] = '\0';
    }
}

// Процесс 2 - обработка данных и передача через очередь сообщений
void process_2(int msgid_receive, int msgid_send, int N) {
    struct message msg;
    char full_data[BUFFER_SIZE * 10];  // Буфер для накопления данных
    int data_index = 0;
    ssize_t bytes_received;
    int end_of_data = 0;  // Флаг для окончания данных

    // Читаем все сообщения из очереди
    while (!end_of_data) {
        bytes_received = msgrcv(msgid_receive, &msg, BUFFER_SIZE, 0, IPC_NOWAIT);  // Используем IPC_NOWAIT

        if (bytes_received > 0) {
            printf("Получено сообщение в процессе 2: %.*s\n", (int)bytes_received, msg.data);  // Отладочный вывод
            memcpy(full_data + data_index, msg.data, bytes_received);
            data_index += bytes_received;
        } else if (bytes_received == -1) {
            // Если очередь пуста, завершаем обработку
            break;
        }
    }

    // Завершаем строку
    full_data[data_index] = '\0';

    // Обрабатываем данные
    char result[BUFFER_SIZE];
    find_last_increasing_sequence(full_data, N, result);

    // Отправляем результат обратно
    msg.msg_type = 1;
    memcpy(msg.data, result, strlen(result) + 1);
    if (msgsnd(msgid_send, &msg, strlen(result) + 1, 0) == -1) {
        perror("Ошибка отправки сообщения");
        exit(1);
    }
    printf("Отправлено обратно в очередь: %s\n", result);  // Отладочный вывод
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Формат вызова: %s <input_file> <output_file> <N>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[3]);
    if (N <= 0) {
        fprintf(stderr, "N должно быть больше 1!.\n");
        return 1;
    }

    // Получаем доступ к очередям сообщений
    int msgid_receive = msgget(MSG_KEY1, 0666 | IPC_CREAT);
    if (msgid_receive == -1) {
        perror("Не удалось создать очередь сообщений для получения");
        exit(1);
    }

    int msgid_send = msgget(MSG_KEY2, 0666 | IPC_CREAT);
    if (msgid_send == -1) {
        perror("Не удалось создать очередь сообщений для отправки");
        exit(1);
    }

    // Процесс 2 - Обработка данных
    process_2(msgid_receive, msgid_send, N);

    // Удаляем очереди сообщений после завершения
    msgctl(msgid_receive, IPC_RMID, NULL);
    msgctl(msgid_send, IPC_RMID, NULL);

    return 0;
}
