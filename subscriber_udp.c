#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_MSG 1024

int main() {
    int sockfd;
    struct sockaddr_in broker_addr, from_addr;
    socklen_t addr_len = sizeof(from_addr);
    char buffer[MAX_MSG];
    char topic[100];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creando socket");
        exit(EXIT_FAILURE);
    }

    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(PORT);
    broker_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Suscriptor UDP iniciado\n");
    printf("Ingrese el topic al que desea suscribirse (ej: partido1): ");
    fgets(topic, sizeof(topic), stdin);
    topic[strcspn(topic, "\n")] = '\0';

    // enviar mensaje de suscripción
    char sub_msg[MAX_MSG];
    snprintf(sub_msg, sizeof(sub_msg), "SUBSCRIBE:%s", topic);
    sendto(sockfd, sub_msg, strlen(sub_msg), 0,
           (struct sockaddr *)&broker_addr, sizeof(broker_addr));

    printf("Suscrito al topic '%s'\n", topic);

    while (1) {
        memset(buffer, 0, MAX_MSG);
        recvfrom(sockfd, buffer, MAX_MSG, 0, (struct sockaddr *)&from_addr, &addr_len);
        printf("📰 [%s] %s\n", topic, buffer);
    }

    close(sockfd);
    return 0;
}
