#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

int sockfd = -1;

// Manejador para cerrar el socket limpiamente
void signal_handler(int sig) {
    printf("\nCerrando subscriber UDP...\n");
    if (sockfd >= 0) {
        close(sockfd);
    }
    exit(0);
}

// Mostrar menú de temas disponibles
void show_topics() {
    printf("\nSubscriber (UDP)\n");
    printf("Topics disponibles:\n");
    printf("1. goles\n");
    printf("2. tarjetas\n");
    printf("3. cambios\n");
    printf("Seleccione un topic (1-3): ");
}

int main() {
    struct sockaddr_in broker_addr, from_addr;
    socklen_t addr_len = sizeof(from_addr);
    char buffer[BUFFER_SIZE];
    char topic[100];
    int option;

    // Manejar señales (Ctrl+C)
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Crear socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creando socket UDP");
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del broker
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(PORT);
    broker_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Seleccionar topic
    show_topics();
    scanf("%d", &option);
    getchar(); // limpiar buffer

    switch (option) {
        case 1:
            strcpy(topic, "goles");
            break;
        case 2:
            strcpy(topic, "tarjetas");
            break;
        case 3:
            strcpy(topic, "cambios");
            break;
        default:
            printf("Opción inválida, usando el tema 'goles' por defecto\n");
            strcpy(topic, "goles");
            break;
    }

    // Enviar mensaje de suscripción
    snprintf(buffer, BUFFER_SIZE, "SUBSCRIBE:%s", topic);
    if (sendto(sockfd, buffer, strlen(buffer), 0,
               (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
        perror("Error enviando suscripción");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Suscrito al topic: %s\n", topic);
    printf("Esperando mensajes del broker UDP en %s:%d...\n", SERVER_IP, PORT);
    printf("-----------------------------------------------\n");

    // Bucle principal de recepción
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                                      (struct sockaddr *)&from_addr, &addr_len);

        if (bytes_received < 0) {
            perror("Error recibiendo mensaje");
            break;
        }

        buffer[bytes_received] = '\0';

        // Obtener timestamp
        time_t now;
        time(&now);
        char *time_str = ctime(&now);
        time_str[strlen(time_str) - 1] = '\0'; // quitar salto de línea

        // Mostrar mensaje con formato
        printf("[%s] %s\n", time_str, buffer);
        fflush(stdout);
    }

    close(sockfd);
    return 0;
}
