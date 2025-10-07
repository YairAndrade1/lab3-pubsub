#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_MSG 1024
#define MAX_SUBS 50
#define MAX_TOPIC 100

typedef struct {
    struct sockaddr_in addr;  // dirección del suscriptor
    char topic[MAX_TOPIC];    // topic al que está suscrito
    int active;
} subscriber_t;

subscriber_t subscribers[MAX_SUBS];

void init_subs() {
    for (int i = 0; i < MAX_SUBS; i++) {
        subscribers[i].active = 0;
        memset(subscribers[i].topic, 0, sizeof(subscribers[i].topic));
    }
}

// registrar un nuevo suscriptor
void add_subscriber(struct sockaddr_in addr, const char *topic) {
    for (int i = 0; i < MAX_SUBS; i++) {
        if (subscribers[i].active &&
            subscribers[i].addr.sin_addr.s_addr == addr.sin_addr.s_addr &&
            subscribers[i].addr.sin_port == addr.sin_port &&
            strcmp(subscribers[i].topic, topic) == 0) {
            return; // ya estaba registrado
        }
    }

    for (int i = 0; i < MAX_SUBS; i++) {
        if (!subscribers[i].active) {
            subscribers[i].addr = addr;
            subscribers[i].active = 1;
            strncpy(subscribers[i].topic, topic, MAX_TOPIC - 1);
            printf("Nuevo suscriptor: %s:%d -> topic '%s'\n",
                   inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), topic);
            return;
        }
    }
    printf("No hay espacio para más suscriptores.\n");
}

// reenviar mensaje solo a subs del mismo topic
void forward_message(int sockfd, const char *topic, const char *message) {
    for (int i = 0; i < MAX_SUBS; i++) {
        if (subscribers[i].active && strcmp(subscribers[i].topic, topic) == 0) {
            sendto(sockfd, message, strlen(message), 0,
                   (struct sockaddr *)&subscribers[i].addr, sizeof(subscribers[i].addr));
        }
    }
    printf("Mensaje reenviado a suscriptores del topic '%s': %s\n", topic, message);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[MAX_MSG];
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

    printf("Broker UDP iniciado en puerto %d\n", PORT);
    init_subs();

    while (1) {
        memset(buffer, 0, MAX_MSG);
        recvfrom(sockfd, buffer, MAX_MSG, 0, (struct sockaddr *)&client_addr, &addr_len);

        if (strncmp(buffer, "SUBSCRIBE:", 10) == 0) {
            char topic[MAX_TOPIC];
            sscanf(buffer + 10, "%99s", topic);
            add_subscriber(client_addr, topic);
        } else if (strncmp(buffer, "MSG:", 4) == 0) {
            char *topic = strtok(buffer + 4, ":");
            char *msg = strtok(NULL, "");
            if (topic && msg) {
                forward_message(sockfd, topic, msg);
            }
        } else {
            printf("Mensaje desconocido: %s\n", buffer);
        }
    }

    close(sockfd);
    return 0;
}
