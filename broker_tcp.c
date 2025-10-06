#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_TOPIC_SIZE 100

// Estructura para mantener información de clientes
typedef struct {
    int socket;
    char type[20]; // "publisher" o "subscriber"
    char topic[MAX_TOPIC_SIZE]; // solo para subscribers
    int active;
} client_t;

client_t clients[MAX_CLIENTS];
int num_clients = 0;

// Función para inicializar la lista de clientes
void init_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = -1;
        clients[i].active = 0;
        memset(clients[i].type, 0, sizeof(clients[i].type));
        memset(clients[i].topic, 0, sizeof(clients[i].topic));
    }
}

// Función para agregar un cliente
int add_client(int socket, const char* type, const char* topic) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].socket = socket;
            clients[i].active = 1;
            strcpy(clients[i].type, type);
            if (topic) {
                strcpy(clients[i].topic, topic);
            }
            num_clients++;
            printf("Cliente agregado: %s (socket: %d, topic: %s)\n", type, socket, topic ? topic : "N/A");
            return i;
        }
    }
    return -1; // No hay mas espacio para clientes
}

// Función para eliminar un cliente
void remove_client(int socket) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == socket && clients[i].active) {
            printf("Cliente desconectado: %s (socket: %d)\n", clients[i].type, socket);
            close(socket);
            clients[i].socket = -1;
            clients[i].active = 0;
            memset(clients[i].type, 0, sizeof(clients[i].type));
            memset(clients[i].topic, 0, sizeof(clients[i].topic));
            num_clients--;
            break;
        }
    }
}

// Función para reenviar mensaje a subscribers del topic
void forward_message(const char* topic, const char* message) {
    printf("Reenviando mensaje del topic '%s': %s\n", topic, message);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && 
            strcmp(clients[i].type, "subscriber") == 0 &&
            strcmp(clients[i].topic, topic) == 0) {
            
            int bytes_sent = send(clients[i].socket, message, strlen(message), 0);
            if (bytes_sent < 0) {
                printf("Error enviando mensaje a subscriber (socket: %d)\n", clients[i].socket);
                remove_client(clients[i].socket);
            } else {
                printf("Mensaje enviado a subscriber (socket: %d)\n", clients[i].socket);
            }
        }
    }
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    fd_set read_fds, master_fds;
    int max_fd;
    char buffer[BUFFER_SIZE];
    
    init_clients();
    
    // Crear socket servidor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creando socket");
        exit(1);
    }
    
    // Configurar reutilización de dirección
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error en setsockopt");
        exit(1);
    }
    
    // Configurar dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        close(server_socket);
        exit(1);
    }
    
    // Listen
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Error en listen");
        close(server_socket);
        exit(1);
    }
    
    printf("Broker TCP iniciado en puerto %d\n", PORT);
    printf("Esperando conexiones...\n");
    
    // Inicializar sets de file descriptors
    FD_ZERO(&master_fds);
    FD_SET(server_socket, &master_fds);
    max_fd = server_socket;
    
    while (1) {
        read_fds = master_fds;
        
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Error en select");
            break;
        }
        
        // Revisar todos los file descriptors
        for (int fd = 0; fd <= max_fd; fd++) {
            if (FD_ISSET(fd, &read_fds)) {
                
                if (fd == server_socket) {
                    // Nueva conexión
                    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
                    if (client_socket < 0) {
                        perror("Error en accept");
                        continue;
                    }
                    
                    FD_SET(client_socket, &master_fds);
                    if (client_socket > max_fd) {
                        max_fd = client_socket;
                    }
                    
                    printf("Nueva conexión desde %s:%d (socket: %d)\n", 
                           inet_ntoa(client_addr.sin_addr), 
                           ntohs(client_addr.sin_port), 
                           client_socket);
                    
                } else {
                    // Datos desde cliente existente
                    memset(buffer, 0, BUFFER_SIZE);
                    int bytes_received = recv(fd, buffer, BUFFER_SIZE - 1, 0);
                    
                    if (bytes_received <= 0) {
                        // Cliente desconectado
                        printf("Cliente desconectado (socket: %d)\n", fd);
                        FD_CLR(fd, &master_fds);
                        remove_client(fd);
                    } else {
                        buffer[bytes_received] = '\0';
                        printf("Mensaje recibido (socket: %d): %s", fd, buffer);
                        
                        // Procesar el mensaje
                        if (strncmp(buffer, "SUBSCRIBE:", 10) == 0) {
                            // Es un subscriber registrándose
                            char topic[MAX_TOPIC_SIZE];
                            sscanf(buffer + 10, "%s", topic);
                            add_client(fd, "subscriber", topic);
                            
                        } else if (strncmp(buffer, "PUBLISH:", 8) == 0) {
                            // Es un publisher registrándose
                            add_client(fd, "publisher", NULL);
                            
                        } else if (strncmp(buffer, "MSG:", 4) == 0) {
                            // Es un mensaje a publicar
                            char topic[MAX_TOPIC_SIZE];
                            char message[BUFFER_SIZE];
                            char* token1 = strtok(buffer + 4, ":");
                            char* token2 = strtok(NULL, "");
                            
                            if (token1 && token2) {
                                strcpy(topic, token1);
                                strcpy(message, token2);
                                forward_message(topic, message);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Cerrar todas las conexiones
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            close(clients[i].socket);
        }
    }
    close(server_socket);
    
    return 0;
}
