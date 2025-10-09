#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

// Función para enviar mensajes UDP
void send_message(int sockfd, struct sockaddr_in *broker_addr, const char *topic, const char *message) {
    char full_message[BUFFER_SIZE];
    snprintf(full_message, sizeof(full_message), "MSG:%s:%s", topic, message);

    int bytes_sent = sendto(sockfd, full_message, strlen(full_message), 0,
                            (struct sockaddr *)broker_addr, sizeof(*broker_addr));
    if (bytes_sent < 0) {
        perror("Error enviando mensaje");
    } else {
        printf("Mensaje enviado: %s\n", full_message);
    }
}

// Menú interactivo
void show_menu() {
    printf("\nPublisher (UDP)\n");
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
    char buffer[BUFFER_SIZE];
    char topic[100];
    char message[BUFFER_SIZE];

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

    printf("Conectando al broker UDP en %s:%d\n", SERVER_IP, PORT);
    printf("Registrado como publisher UDP\n");

    while (1) {
        int option = 0;
        show_menu();
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            continue;
        }
        option = atoi(buffer);

        if (option == 4) {
            printf("Cerrando publisher...\n");
            break;
        }

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
                printf("Opción no válida. Intenta de nuevo.\n");
                continue;
        }

        printf("Mensaje: ");
        if (fgets(message, sizeof(message), stdin) == NULL) {
            continue;
        }
        if (strncmp(message, "salir", 5) == 0 || strncmp(message, "exit", 4) == 0) {
            printf("Cerrando publisher...\n");
            break;
        }

        send_message(sockfd, &broker_addr, topic, message);
        printf("Mensaje enviado al topic '%s'\n", topic);
    }

    close(sockfd);
    return 0;
}
