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
    for (int i = 0; i < MAX_SUBS; i++) {
        if (subscribers[i].active &&
            subscribers[i].addr.sin_addr.s_addr == addr.sin_addr.s_addr &&
            subscribers[i].addr.sin_port == addr.sin_port &&
            strcmp(subscribers[i].topic, topic) == 0) {
            return; // Ya registrado
        }
    }

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
