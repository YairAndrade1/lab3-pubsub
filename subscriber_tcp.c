#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

int client_socket = -1;

// Manejar señales para cerrar limpiamente 
void signal_handler(int sig) {
    printf("\nCerrando subscriber...\n");
    if (client_socket >= 0) {
        close(client_socket);
    }
    exit(0);
}

void show_topics() {
    printf("\nSubscriber (TCP)\n");
    printf("Topics disponibles:\n");
    printf("1. goles\n");
    printf("2. tarjetas\n");
    printf("3. cambios\n");
    printf("Seleccione un topic (1-3): ");
}

int main() {
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char topic[100];
    int option;
    
    // Configurar manejador de señales
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
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
        perror("Dirección inválida");
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
    
    // Seleccionar topic
    show_topics();
    scanf("%d", &option);
    getchar(); 
    
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
    
    // Suscribirse al topic
    snprintf(buffer, BUFFER_SIZE, "SUBSCRIBE:%s\n", topic);
    if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
        perror("Error enviando suscripción");
        close(client_socket);
        exit(1);
    }
    
    printf("Suscrito al topic: %s\n", topic);
    printf("Esperando mensajes...\n");
    printf("-----------------------------------------\n");
    
    // Bucle para recibir mensajes
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received < 0) {
            perror("Error recibiendo mensaje");
            break;
        } else if (bytes_received == 0) {
            printf("Conexión cerrada por el broker\n");
            break;
        } else {
            buffer[bytes_received] = '\0';
            
            // Mostrar mensaje con timestamp
            time_t now;
            time(&now);
            char* time_str = ctime(&now);
            time_str[strlen(time_str) - 1] = '\0'; 
            
            printf("[%s] %s", time_str, buffer);
            fflush(stdout);
        }
    }
    
    close(client_socket);
    return 0;
}
