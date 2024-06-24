#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 80

void save_to_file(const char *filename, const char *data);
void download_resource(const char *host, const char *recurso, int level);
int is_empty(const char *path);
int has_been_processed(const char *resource);
void mark_as_processed(const char *resource);

typedef struct ProcessedNode {
    char *resource;
    struct ProcessedNode *next;
} ProcessedNode;

ProcessedNode *processed_list = NULL;

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
            if (!*n) {
                return (char *)haystack;
            }
        }
    }
    return NULL;
}

void create_directory(const char *path) {
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "mkdir -p \"%s\"", path);
    if (system(command) != 0) {
        fprintf(stderr, "Error al crear el directorio %s: %s\n", path, strerror(errno));
    }
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
    if (strstr(entrada, http) == entrada) {
        aux_entrada = entrada + strlen(http); // Avanzar más allá del protocolo
    } else if (strstr(entrada, https) == entrada) {
        aux_entrada = entrada + strlen(https); // Avanzar más allá del protocolo
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

    // Descargar el recurso
    download_resource(direccion, recurso, 0);

    free(direccion);
    close(sockfd);

    return 0;
}

// Función para guardar la respuesta en un archivo
void save_to_file(const char *filename, const char *data) {
    // Verificar que el nombre del archivo no sea demasiado largo
    if (strlen(filename) >= BUFFER_SIZE) {
        fprintf(stderr, "Error: Nombre de archivo demasiado largo: %s\n", filename);
        return;
    }

    // Crear el directorio para el archivo si no existe
    char dir_name[BUFFER_SIZE];
    char *last_slash = strrchr(filename, '/');
    if (last_slash != NULL) {
        size_t dir_len = last_slash - filename;
        strncpy(dir_name, filename, dir_len);
        dir_name[dir_len] = '\0';
        create_directory(dir_name);
    }

    // Guardar el contenido en el archivo
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        error("Error al abrir el archivo para escribir");
    }
    fwrite(data, 1, strlen(data), file);
    fclose(file);
}

// Función para verificar si un archivo o directorio está vacío
int is_empty(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            // Es un directorio, verificar si está vacío
            int n = 0;
            struct dirent *d;
            DIR *dir = opendir(path);
            if (dir == NULL) return 1; // Si no se puede abrir, considerar vacío
            while ((d = readdir(dir)) != NULL) {
                if (++n > 2) break; // Directorio no está vacío (contiene más de '.' y '..')
            }
            closedir(dir);
            return n <= 2;
        } else if (S_ISREG(st.st_mode)) {
            // Es un archivo, verificar si tiene tamaño cero
            return st.st_size == 0;
        }
    }
    return 1; // Si no se puede obtener información, considerar vacío
}

// Función para verificar si un recurso ya ha sido procesado
int has_been_processed(const char *resource) {
    ProcessedNode *current = processed_list;
    while (current != NULL) {
        if (strcmp(current->resource, resource) == 0) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

// Función para marcar un recurso como procesado
void mark_as_processed(const char *resource) {
    ProcessedNode *new_node = (ProcessedNode *)malloc(sizeof(ProcessedNode));
    new_node->resource = strdup(resource);
    new_node->next = processed_list;
    processed_list = new_node;
}

// Función para descargar un recurso
void download_resource(const char *host, const char *recurso, int level) {
    // Ignorar archivos con la extensión .swf
    const char *ext = strrchr(recurso, '.');
    if (ext && strcmp(ext, ".swf") == 0) {
        return;
    }

    // Verificar si el recurso ya ha sido procesado
    if (has_been_processed(recurso)) {
        return;
    }

    // Marcar el recurso como procesado
    mark_as_processed(recurso);

    // Crear el socket
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
    //printf("Recurso: %s\n", recurso);

    if (send(sockfd, request, strlen(request), 0) < 0)
        error("Error al enviar la solicitud");

    char buffer[BUFFER_SIZE];
    int bytes_received;
    char response[BUFFER_SIZE * 100] = {0}; // Buffer para almacenar la respuesta completa
    char *body_start = NULL;
    int body_started = 0;
    int status_code = 0;
    char *content_type;

    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        if (!body_started) {
            // Buscar el comienzo del cuerpo
            body_start = strstr(buffer, "\r\n\r\n");
            if (body_start != NULL) {
                body_started = 1;
                body_start += 4; // Saltar los caracteres \r\n\r\n

                // Procesar los encabezados para obtener el código de estado y el tipo MIME
                char *status_line = strtok(buffer, "\r\n");
                if (status_line != NULL) {
                    sscanf(status_line, "HTTP/1.1 %d", &status_code);
                }

                // Obtener el encabezado Content-Type
                char *header_line = status_line + strlen(status_line) + 1;
                while (header_line < body_start) {
                    if (strstr(header_line, "Content-Type:") != NULL) {
                        content_type = strstr(header_line, "Content-Type: ") + strlen("Content-Type: ");
                        sscanf(header_line, "Content-Type: %s", content_type);
                        break;
                    }
                    header_line += strlen(header_line) + 1;
                }

                // Si el código de estado no es 200, salir
                if (status_code != 200) {
                    fprintf(stderr, "Error: el servidor respondió con el código de estado %d. Intentando obtener recurso '%s'\n", status_code, recurso);
                    close(sockfd);
                    return;
                }

                // Verificar el tipo MIME
                if (strncmp(content_type, "text/html", strlen("text/html")) != 0 && strncmp(content_type, "text/plain", strlen("text/plain")) != 0) {
                    content_type[10] = '\0';
                    //printf("Ignorando el recurso de tipo MIME no permitido: %s\n", content_type);
                    close(sockfd);
                    return;
                }

                //printf("Recurso '%s' obtenido correctamente.\n", recurso);

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

    // Verificar si es un directorio HTML
    if (strstr(response, "<html") != NULL && strstr(response, "<body") != NULL) {
        // Crear el directorio correspondiente si es un directorio
        create_directory(recurso + 1);  // Quitar el '/' inicial para crear un directorio relativo

        // Construir la ruta completa del archivo index.html
        char file_path[BUFFER_SIZE];
        snprintf(file_path, sizeof(file_path), "./%s/index.html", recurso + 1);

        // Guardar el contenido en el archivo index.html dentro del directorio
        save_to_file(file_path, response);
    } else {
        // Si no es HTML, asumir que es un archivo y guardarlo directamente
        char file_path[BUFFER_SIZE];
        snprintf(file_path, sizeof(file_path), "./%s", recurso); // Guardar en la ruta correspondiente
        save_to_file(file_path, response);
    }

    // Analizar y descargar recursos adicionales si es HTML
    if (strstr(response, "<!DOCTYPE html>") != NULL || strstr(response, "<html") != NULL) {
        char *ptr = response;
        char *ptr_aux;
        while ((ptr_aux = strcasestr(ptr, "href=\"")) != NULL || (ptr_aux = strcasestr(ptr, "src=\"")) != NULL) {
            ptr = ptr_aux;
            ptr += (ptr_aux[0] == 'h') ? 6 : 5; // Avanzar más allá de "href=\"" o "src=\""
            char *end = strchr(ptr, '"');
            if (end == NULL) break;
            char link[BUFFER_SIZE];
            strncpy(link, ptr, end - ptr);
            link[end - ptr] = '\0';

            if (strstr(link, "?") == NULL && strstr(link, "http") == NULL) { // Solo manejar enlaces relativos, ignorando '?'
                char new_resource[BUFFER_SIZE * 2];

                if (recurso[0] != link[0]) {
                    snprintf(new_resource, BUFFER_SIZE * 2, "%s/%s", recurso, link);
                }
                else {
                    snprintf(new_resource, BUFFER_SIZE * 2, "%s", link);
                }

                // Descargar el recurso solo si no se ha descargado antes para evitar duplicados
                if (is_empty(new_resource)) {
                    download_resource(host, new_resource, level + 1);
                }
            }

            ptr = end + 1;
        }
    }
}
