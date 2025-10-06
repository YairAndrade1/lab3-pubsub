#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

void send_message(int socket, const char* topic, const char* message) {
    char full_message[BUFFER_SIZE];
    snprintf(full_message, BUFFER_SIZE, "MSG:%s:%s", topic, message);
    
    int bytes_sent = send(socket, full_message, strlen(full_message), 0);
    if (bytes_sent < 0) {
        perror("Error enviando mensaje");
    } else {
        printf("Mensaje enviado: %s\n", full_message);
    }
}

void show_menu() {
    printf("\nPublisher (TCP)\n");
    printf("Seleccione un tema:\n");
    printf("1. goles\n");
    printf("2. tarjetas\n");
    printf("3. cambios\n");
    printf("4. salir\n");
    printf("Opcion: ");
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char topic[100];
    char message[BUFFER_SIZE];
    
    // Crear socket cliente
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Error creando socket");
        exit(1);
    }
    
    // Configurar dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Dirección no valida");
        close(client_socket);
        exit(1);
    }
    
    // Conectar al broker
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error conectando al broker");
        close(client_socket);
        exit(1);
    }
    
    printf("Conectado al broker en %s:%d\n", SERVER_IP, PORT);
    
    // Registrarse como publisher
    strcpy(buffer, "PUBLISH:\n");
    send(client_socket, buffer, strlen(buffer), 0);
    printf("Registrado como publisher\n");
    
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
                printf("Opción no valida. Intenta de nuevo.\n");
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
        send_message(client_socket, topic, message);
        printf("Mensaje enviado al topic '%s'\n", topic);
    }
    
    close(client_socket);
    return 0;
}
