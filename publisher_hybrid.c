// publisher_hybrid.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 8080
#define BUFFER_SIZE 2048
#define SERVER_IP "127.0.0.1"
#define TIMEOUT_SEC 2      // tiempo de espera por respuesta (segundos)
#define MAX_ATTEMPTS 3     // reintentos si no hay respuesta (opcional)

void show_menu() {
    printf("\nPublisher (Híbrido - UDP con ACKs)\n");
    printf("Seleccione un tema:\n");
    printf("1. goles\n");
    printf("2. tarjetas\n");
    printf("3. cambios\n");
    printf("4. salir\n");
    printf("Opcion: ");
}

int main() {
    int sockfd;
    struct sockaddr_in broker_addr;
    char line[BUFFER_SIZE];
    char topic[128];
    char message[BUFFER_SIZE];
    char packet[BUFFER_SIZE];

    // Crear socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creando socket UDP");
        exit(EXIT_FAILURE);
    }

    // Configurar broker
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(PORT);
    broker_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // configurar timeout para recvfrom
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
        // no fatal: seguimos sin timeout si falla
    }

    printf("Publisher híbrido iniciado (broker %s:%d)\n", SERVER_IP, PORT);
    printf("Registrado como publisher (UDP)\n");

    while (1) {
        int option = 0;
        show_menu();
        if (fgets(line, sizeof(line), stdin) == NULL) continue;
        option = atoi(line);

        if (option == 4) {
            printf("Cerrando publisher...\n");
            break;
        }

        switch (option) {
            case 1: strcpy(topic, "goles"); break;
            case 2: strcpy(topic, "tarjetas"); break;
            case 3: strcpy(topic, "cambios"); break;
            default:
                printf("Opción no válida. Intenta de nuevo.\n");
                continue;
        }

        printf("Mensaje: ");
        if (fgets(message, sizeof(message), stdin) == NULL) continue;
        // quitar '\n' final
        message[strcspn(message, "\n")] = '\0';
        if (strcmp(message, "exit") == 0 || strcmp(message, "salir") == 0) {
            printf("Cerrando publisher...\n");
            break;
        }

        // Construir paquete con formato MSG:topic:mensaje
        snprintf(packet, sizeof(packet), "MSG:%s:%s", topic, message);

        // Enviar y (opcional) esperar ACK/respuesta del broker
        int attempt;
        for (attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
            ssize_t sent = sendto(sockfd, packet, strlen(packet), 0,
                                  (struct sockaddr *)&broker_addr, sizeof(broker_addr));
            if (sent < 0) {
                perror("Error enviando mensaje");
                break;
            }
            printf("Mensaje enviado (attempt %d): %s\n", attempt + 1, packet);

            // Esperar respuesta (ej: broker puede enviar "ACK:RECEIVED" o similar)
            char resp[BUFFER_SIZE];
            struct sockaddr_in from;
            socklen_t fromlen = sizeof(from);
            ssize_t r = recvfrom(sockfd, resp, sizeof(resp)-1, 0, (struct sockaddr *)&from, &fromlen);

            if (r > 0) {
                resp[r] = '\0';
                // Mostrar cualquier respuesta del broker
                printf("Respuesta recibida desde %s:%d -> %s\n",
                       inet_ntoa(from.sin_addr), ntohs(from.sin_port), resp);
                // si el broker responde con un ACK de recepción, paramos los reintentos
                if (strncmp(resp, "ACK", 3) == 0) {
                    break;
                }
                // si recibe otra cosa, igual rompemos (ya tenemos respuesta)
                break;
            } else {
                // recvfrom falló por timeout (valor de r == -1) o no hubo respuesta
                if (attempt < MAX_ATTEMPTS - 1) {
                    printf("No se recibió ACK, reintentando...\n");
                } else {
                    printf("No se recibió ACK tras %d intentos. Continuando...\n", MAX_ATTEMPTS);
                }
            }
        } // for attempts

        // opcional: si deseas no hacer reintentos en absoluto, pon MAX_ATTEMPTS = 1
    }

    close(sockfd);
    return 0;
}
