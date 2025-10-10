#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 2048
#define SERVER_IP "127.0.0.1"

int sockfd = -1;
struct sockaddr_in broker_addr;

// Manejar señales para cierre limpio
void signal_handler(int sig) {
    printf("\nCerrando subscriber...\n");
    if (sockfd >= 0) close(sockfd);
    exit(0);
}

void show_topics() {
    printf("\nSubscriber (Híbrido UDP + ACKs)\n");
    printf("Topics disponibles:\n");
    printf("1. goles\n");
    printf("2. tarjetas\n");
    printf("3. cambios\n");
    printf("4. salir\n");
    printf("Seleccione un topic (1-3): ");
}

void subscribe_to_topic(const char *topic) {
    char msg[BUFFER_SIZE];
    snprintf(msg, sizeof(msg), "SUBSCRIBE:%s", topic);
    sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&broker_addr, sizeof(broker_addr));
    printf("Suscripción enviada al topic '%s'\n", topic);
}

int main() {
    char buffer[BUFFER_SIZE];
    char topic[100];
    int option;
    socklen_t addr_len = sizeof(broker_addr);

    // Manejar señales
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Crear socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creando socket UDP");
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del broker
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(PORT);
    broker_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Mostrar menú y seleccionar topic
    show_topics();

    while (1) {
        if (scanf("%d", &option) != 1) {
            printf("Entrada inválida. Intente de nuevo: ");
            while (getchar() != '\n'); // limpiar buffer
            continue;
        }

        if (option == 4) {
            printf("Saliendo...\n");
            close(sockfd);
            return 0;
        }

        if (option < 1 || option > 3) {
            printf("Opción inválida. Seleccione entre 1 y 3: ");
            continue;
        }

        // Asignar el topic según la opción
        switch (option) {
            case 1: strcpy(topic, "goles"); break;
            case 2: strcpy(topic, "tarjetas"); break;
            case 3: strcpy(topic, "cambios"); break;
        }
        break;
    }

    // Enviar suscripción
    subscribe_to_topic(topic);
    printf("Suscrito al topic '%s'\n", topic);
    printf("Esperando mensajes...\n");
    printf("-----------------------------------------\n");

    // Recibir mensajes
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, NULL, NULL);
        if (bytes < 0) {
            perror("Error recibiendo mensaje");
            continue;
        }

        buffer[bytes] = '\0';
        time_t now;
        time(&now);
        char *time_str = ctime(&now);
        time_str[strlen(time_str) - 1] = '\0';

        // Procesar formato MSGID:<id>:MSG:<topic>:<mensaje>
        if (strncmp(buffer, "MSGID:", 6) == 0) {
            int msg_id;
            char recv_topic[100], msg[BUFFER_SIZE];
            if (sscanf(buffer, "MSGID:%d:MSG:%99[^:]:%1023[^\n]", &msg_id, recv_topic, msg) == 3) {
                printf("[%s] Mensaje [%d] en topic '%s': %s\n",
                       time_str, msg_id, recv_topic, msg);

                // Enviar ACK al broker
                char ack[100];
                snprintf(ack, sizeof(ack), "ACK:MSGID:%d", msg_id);
                sendto(sockfd, ack, strlen(ack), 0, (struct sockaddr *)&broker_addr, sizeof(broker_addr));
                printf("[%s] ACK enviado para mensaje %d\n", time_str, msg_id);
            }
        } else if (strncmp(buffer, "ACK:SUBSCRIBE:", 14) == 0) {
            printf("[%s] Confirmación de suscripción: %s\n", time_str, buffer);
        } else if (strncmp(buffer, "ACK:RECEIVED", 12) == 0) {
            printf("[%s] Broker confirmó recepción\n", time_str);
        } else {
            printf("[%s] Mensaje desconocido: %s\n", time_str, buffer);
        }
    }

    close(sockfd);
    return 0;
}
