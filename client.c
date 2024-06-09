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
    char *direccion = NULL;
    char *recurso;
    int puerto = DEFAULT_PORT;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <[protocolo://]direccion[:puerto][/recurso]>\n", argv[0]);
        exit(1);
    }

    char *entrada = argv[1];
    char *protocolo = "http://";
    char *aux_entrada;

    // Verificar si se proporcionó el protocolo en la entrada
    if (strstr(entrada, protocolo) == entrada) {
        aux_entrada = entrada + strlen(protocolo); // Avanzar más allá del protocolo
    } else {
        aux_entrada = entrada; // No se proporcionó el protocolo, asumir que es HTTP
    }

    // Verificar si se proporcionó el puerto
    char *portDelimiter = strchr(aux_entrada, ':');
    if (portDelimiter != NULL) {
        direccion = (char*) malloc((portDelimiter - aux_entrada + 1) * sizeof(char));
        strncpy(direccion, aux_entrada, portDelimiter - aux_entrada);
        direccion[portDelimiter - aux_entrada] = '\0';
        puerto = atoi(portDelimiter + 1);
        aux_entrada = portDelimiter + 1;
    }

    // Extraer la ruta del recurso
    char *pathDelimiter = strchr(aux_entrada, '/');
    if (direccion == NULL && pathDelimiter != NULL) {
        direccion = (char*) malloc((pathDelimiter - aux_entrada + 1) * sizeof(char));
        strncpy(direccion, aux_entrada, pathDelimiter - aux_entrada);
        direccion[pathDelimiter - aux_entrada] = '\0';
    }
    if (pathDelimiter != NULL) {
        recurso = pathDelimiter;
    } else {
        direccion = (char*) malloc(sizeof(aux_entrada));
        strcpy(direccion, aux_entrada);
        recurso = "/";
    }

    if (puerto != 80) {
        error("Error de puerto no válido");
    }

    // Crear el socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Error al abrir el socket");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(puerto);
    inet_pton(AF_INET, direccion, &serv_addr.sin_addr);

    // Conectar al servidor
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("Error al conectar");

    // Construir la solicitud HTTP
    char request[BUFFER_SIZE];
    snprintf(request, BUFFER_SIZE, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nAccept: text/html\r\n\r\n", recurso, direccion);

    // Enviar la solicitud al servidor
    if (send(sockfd, request, strlen(request), 0) < 0)
        error("Error al enviar la solicitud");

    // Recibir y mostrar la respuesta del servidor
    char buffer[BUFFER_SIZE];
    int bytes_received;
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }

    if (bytes_received < 0)
        error("Error al recibir la respuesta");

    free(direccion);
    close(sockfd);

    return 0;
}
