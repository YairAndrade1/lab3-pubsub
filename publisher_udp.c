#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_MSG 1024

int main() {
    int sockfd;
    struct sockaddr_in broker_addr;
    char buffer[MAX_MSG];
    char topic[100];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creando socket");
        exit(EXIT_FAILURE);
    }

    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(PORT);
    broker_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Publisher UDP iniciado\n");
    printf("Ingrese el topic (ej: partido1): ");
    fgets(topic, sizeof(topic), stdin);
    topic[strcspn(topic, "\n")] = '\0';

    while (1) {
        printf("Escriba mensaje (o 'exit'): ");
        fgets(buffer, MAX_MSG, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        if (strcmp(buffer, "exit") == 0)
            break;

        char msg[MAX_MSG + 100];
        snprintf(msg, sizeof(msg), "MSG:%s:%s", topic, buffer);

        sendto(sockfd, msg, strlen(msg), 0,
               (struct sockaddr *)&broker_addr, sizeof(broker_addr));
    }

    close(sockfd);
    return 0;
}
