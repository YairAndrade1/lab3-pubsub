#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 8080
#define BUFFER_SIZE 2048
#define SERVER_IP "127.0.0.1"
#define TIMEOUT_SEC 5

void subscribe_to_topic(int sockfd, struct sockaddr_in *broker_addr, const char *topic) {
    char msg[BUFFER_SIZE];
    snprintf(msg, sizeof(msg), "SUBSCRIBE:%s", topic);
    sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)broker_addr, sizeof(*broker_addr));
    printf("📡 Enviada suscripción al topic '%s'\n", topic);
}

int main() {
    int sockfd;
    struct sockaddr_in broker_addr, from;
    char buffer[BUFFER_SIZE];
    socklen_t from_len = sizeof(from);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creando socket");
        exit(EXIT_FAILURE);
    }

    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(PORT);
    broker_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Timeout opcional (por si no llegan mensajes)
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    char topic[100];
    printf("\nSubscriber (Híbrido UDP + ACKs)\n");
    printf("Ingrese el topic al que desea suscribirse: ");
    scanf("%99s", topic);
    getchar();

    subscribe_to_topic(sockfd, &broker_addr, topic);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&from, &from_len);

        if (bytes < 0) {
            printf("⏳ Esperando mensajes del broker...\n");
            continue;
        }

        buffer[bytes] = '\0';
        printf("📥 Recibido: %s\n", buffer);

        if (strncmp(buffer, "MSGID:", 6) == 0) {
            int msg_id;
            char recv_topic[100], msg[BUFFER_SIZE];

            // Formato esperado: MSGID:<id>:MSG:<topic>:<mensaje>
            if (sscanf(buffer, "MSGID:%d:MSG:%99[^:]:%1023[^\n]", &msg_id, recv_topic, msg) == 3) {
                printf("🗞️  Mensaje [%d] en topic '%s': %s\n", msg_id, recv_topic, msg);

                // Enviar ACK al broker
                char ack[100];
                snprintf(ack, sizeof(ack), "ACK:MSGID:%d", msg_id);
                sendto(sockfd, ack, strlen(ack), 0, (struct sockaddr *)&broker_addr, sizeof(broker_addr));
                printf("✅ ACK enviado para mensaje %d\n", msg_id);
            }
        } else if (strncmp(buffer, "ACK:SUBSCRIBE:", 14) == 0) {
            printf("🔔 Confirmación de suscripción recibida: %s\n", buffer);
        } else if (strncmp(buffer, "ACK:RECEIVED", 12) == 0) {
            printf("ℹ️  Broker confirmó recepción general\n");
        } else {
            printf("⚠️ Mensaje desconocido: %s\n", buffer);
        }
    }

    close(sockfd);
    return 0;
}
