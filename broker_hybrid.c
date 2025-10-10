#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_SUBS 50
#define MAX_TOPIC 100

typedef struct {
    struct sockaddr_in addr;
    char topic[MAX_TOPIC];
    int active;
    int last_ack_id;  // control de ACKs
} subscriber_t;

subscriber_t subscribers[MAX_SUBS];

void init_subs() {
    for (int i = 0; i < MAX_SUBS; i++) {
        subscribers[i].active = 0;
        subscribers[i].last_ack_id = 0;
        memset(subscribers[i].topic, 0, sizeof(subscribers[i].topic));
    }
}

void add_subscriber(struct sockaddr_in addr, const char *topic, int sockfd) {
    // Verificar si ya existe
    for (int i = 0; i < MAX_SUBS; i++) {
        if (subscribers[i].active &&
            subscribers[i].addr.sin_addr.s_addr == addr.sin_addr.s_addr &&
            subscribers[i].addr.sin_port == addr.sin_port &&
            strcmp(subscribers[i].topic, topic) == 0) {
            return; // Ya registrado
        }
    }

    // Agregar nuevo
    for (int i = 0; i < MAX_SUBS; i++) {
        if (!subscribers[i].active) {
            subscribers[i].addr = addr;
            subscribers[i].active = 1;
            strncpy(subscribers[i].topic, topic, MAX_TOPIC - 1);
            printf("Nuevo subscriber: %s:%d -> topic '%s'\n",
                   inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), topic);

            // Enviar ACK de suscripción
            char ack_msg[BUFFER_SIZE];
            snprintf(ack_msg, sizeof(ack_msg), "ACK:SUBSCRIBE:%s", topic);
            sendto(sockfd, ack_msg, strlen(ack_msg), 0,
                   (struct sockaddr *)&addr, sizeof(addr));
            return;
        }
    }

    printf("No hay espacio para más suscriptores.\n");
}

void forward_message(int sockfd, const char *topic, const char *message) {
    char msg_with_id[BUFFER_SIZE];
    static int message_id = 0;
    message_id++;

    snprintf(msg_with_id, sizeof(msg_with_id), "MSGID:%d:MSG:%s:%s", message_id, topic, message);

    for (int i = 0; i < MAX_SUBS; i++) {
        if (subscribers[i].active && strcmp(subscribers[i].topic, topic) == 0) {
            int sent = sendto(sockfd, msg_with_id, strlen(msg_with_id), 0,
                              (struct sockaddr *)&subscribers[i].addr, sizeof(subscribers[i].addr));
            if (sent < 0)
                perror("Error enviando mensaje a subscriber");
            else
                printf("Enviado a %s:%d -> '%s'\n",
                       inet_ntoa(subscribers[i].addr.sin_addr),
                       ntohs(subscribers[i].addr.sin_port), msg_with_id);
        }
    }
}

void process_ack(const char *buffer, struct sockaddr_in client) {
    int msg_id;
    if (sscanf(buffer, "ACK:MSGID:%d", &msg_id) == 1) {
        printf("ACK recibido de %s:%d por mensaje %d\n",
               inet_ntoa(client.sin_addr), ntohs(client.sin_port), msg_id);
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creando socket UDP");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }

    printf("Broker híbrido (UDP + ACKs) escuchando en puerto %d\n", PORT);
    init_subs();

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                             (struct sockaddr *)&client_addr, &addr_len);

        if (bytes < 0)
            continue;

        buffer[bytes] = '\0';
        printf("Recibido de %s:%d: %s\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);

        sendto(sockfd, "ACK:RECEIVED", strlen("ACK:RECEIVED"), 0,
               (struct sockaddr *)&client_addr, addr_len);

        if (strncmp(buffer, "SUBSCRIBE:", 10) == 0) {
            char topic[MAX_TOPIC];
            sscanf(buffer + 10, "%99s", topic);
            add_subscriber(client_addr, topic, sockfd);
        } else if (strncmp(buffer, "MSG:", 4) == 0) {
            char *topic = strtok(buffer + 4, ":");
            char *msg = strtok(NULL, "");
            if (topic && msg) {
                forward_message(sockfd, topic, msg);
            }
        } else if (strncmp(buffer, "ACK:MSGID:", 10) == 0) {
            process_ack(buffer, client_addr);
        } else {
            printf("Mensaje desconocido: %s\n", buffer);
        }
    }

    close(sockfd);
    return 0;
}
