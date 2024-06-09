#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 80

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    char *servidor;
    int puerto;

    if (argc == 2) {
        servidor = argv[1];
        puerto = DEFAULT_PORT;
    } else if (argc == 3) {
        servidor = argv[1];
        puerto = atoi(argv[2]);
    } else {
        fprintf(stderr, "Uso: %s <servidor> [puerto]\n", argv[0]);
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Error al abrir el socket");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(puerto);
    inet_pton(AF_INET, servidor, &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("Error al conectar");

    char *request = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\nAccept: text/html\r\n\r\n";
    if (send(sockfd, request, strlen(request), 0) < 0)
        error("Error al enviar la solicitud");

    char buffer[BUFFER_SIZE];
    int bytes_received;

    // Lee la respuesta del servidor mientras haya datos
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }

    if (bytes_received < 0)
        error("Error al recibir la respuesta");

    close(sockfd);

    return 0;
}
