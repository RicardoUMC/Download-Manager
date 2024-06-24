#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ctype.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 80

void save_to_file(const char *filename, const char *data);
void download_resource(const char *host, const char *recurso, int level);

void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Función para encontrar una subcadena insensible a mayúsculas y minúsculas
char *strcasestr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for ( ; *haystack; ++haystack) {
        if (tolower(*haystack) == tolower(*needle)) {
            const char *h, *n;
            for (h = haystack, n = needle; *h && *n; ++h, ++n) {
                if (tolower(*h) != tolower(*n)) break;
            }
            if (!*n) return (char *)haystack;
        }
    }
    return NULL;
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
    char *http = "http://";
    char *https = "https://";
    char *aux_entrada;

    // Verificar si se proporcionó el protocolo en la entrada
    if (strstr(entrada, http) == entrada ) {
        aux_entrada = entrada + strlen(http); // Avanzar más allá del protocolo
    }
    else if (strstr(entrada, https) == entrada ) {
        aux_entrada = entrada + strlen(https); // Avanzar más allá del protocolo
    }
    else {
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
    /*char buffer[BUFFER_SIZE];
    int bytes_received;
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }

    if (bytes_received < 0)
        error("Error al recibir la respuesta");
    */
    download_resource(direccion, recurso, 0);

    free(direccion);
    close(sockfd);

    return 0;
}

// Función para guardar la respuesta en un archivo
void save_to_file(const char *filename, const char *data) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        error("Error al abrir el archivo para escribir");
    }
    fwrite(data, 1, strlen(data), file);
    fclose(file);
}

// Función para descargar un recurso
void download_resource(const char *host, const char *recurso, int level) {
    if (level > 2) return; // Acotar a 2 niveles de profundidad

    int puerto = DEFAULT_PORT;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Error al abrir el socket");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(puerto);
    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
        error("Error al convertir la dirección IP");
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("Error al conectar");

    char request[BUFFER_SIZE];
    snprintf(request, BUFFER_SIZE, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nAccept: text/html\r\n\r\n", recurso, host);

    if (send(sockfd, request, strlen(request), 0) < 0)
        error("Error al enviar la solicitud");

    char buffer[BUFFER_SIZE];
    int bytes_received;
    char response[BUFFER_SIZE * 10] = {0}; // Buffer para almacenar la respuesta completa
    char *body_start = NULL;
    int body_started = 0;
    int status_code = 0;

    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        if (!body_started) {
            // Buscar el comienzo del cuerpo
            body_start = strstr(buffer, "\r\n\r\n");
            if (body_start != NULL) {
                body_started = 1;
                body_start += 4; // Saltar los caracteres \r\n\r\n

                // Procesar los encabezados para obtener el código de estado
                char *status_line = strtok(buffer, "\r\n");
                if (status_line != NULL) {
                    sscanf(status_line, "HTTP/1.1 %d", &status_code);
                }

                // Si el código de estado no es 200, salir
                if (status_code != 200) {
                    fprintf(stderr, "Error: el servidor respondió con el código de estado %d\n", status_code);
                    close(sockfd);
                    return;
                }

                // Agregar el cuerpo recibido después de los encabezados al buffer de respuesta
                strncat(response, body_start, bytes_received - (body_start - buffer));
            }
        } else {
            // Ya se ha encontrado la línea en blanco, agregar el resto del contenido al buffer de respuesta
            strncat(response, buffer, bytes_received);
        }
    }

    if (bytes_received < 0)
        error("Error al recibir la respuesta");

    close(sockfd);

    // Extraer el nombre del archivo desde el recurso
    char *filename = basename((char *)recurso);
    if (strcmp(filename, "/") == 0) {
        filename = "index.html";
    }

    save_to_file(filename, response);

    // Analizar y descargar recursos adicionales si es HTML
    if (strstr(response, "<!DOCTYPE html>") != NULL || strstr(response, "<html") != NULL) {
        char *ptr = response;
        while ((ptr = strcasestr(ptr, "href=\"")) != NULL || (ptr = strcasestr(ptr, "src=\"")) != NULL) {
            ptr += 6;
            char *end = strchr(ptr, '"');
            if (end == NULL) break;
            char link[BUFFER_SIZE];
            strncpy(link, ptr, end - ptr);
            link[end - ptr] = '\0';

            if (strstr(link, "http") == NULL) { // Solo manejar enlaces relativos
                char new_resource[BUFFER_SIZE];
                snprintf(new_resource, BUFFER_SIZE, "%s", link);
                download_resource(host, new_resource, level + 1);
            }

            ptr = end + 1;
        }
    }
}

