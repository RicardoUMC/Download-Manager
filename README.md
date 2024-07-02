# Web Resource Downloader

Este proyecto es un programa en C que descarga recursos web desde una URL específica y guarda el contenido en el sistema de archivos local. Soporta descargas recursivas de recursos enlazados en páginas HTML.

## Requisitos

- Un compilador de C (por ejemplo, `gcc`)
- Biblioteca estándar de C
- Biblioteca de sockets POSIX

## Compilación

Para compilar el programa, ejecuta el siguiente comando en tu terminal:

```bash
gcc -o web_downloader web_downloader.c
```

Esto generará un ejecutable llamado `web_downloader`.

## Ejecución

Para ejecutar el programa, utiliza el siguiente comando:

```bash
./web_downloader <[protocolo://]direccion[:puerto][/recurso]>
```

### Ejemplos

1. Descargar el recurso principal de `example.com`:
    ```bash
    ./web_downloader http://example.com
    ```

2. Descargar un recurso específico desde `example.com`:
    ```bash
    ./web_downloader http://example.com/resource
    ```

## Probar el Programa

Puedes probar el programa utilizando cualquier URL válida. Aquí hay algunos ejemplos de prueba:

1. Descargar el contenido principal de `example.com`:
    ```bash
    ./web_downloader http://example.com
    ```

2. Descargar un recurso específico desde `example.com`:
    ```bash
    ./web_downloader http://example.com/index.html
    ```

3. Descargar desde una dirección sin especificar el protocolo (asume HTTP por defecto):
    ```bash
    ./web_downloader example.com
    ```

4. Descargar desde un recurso en un puerto específico (el puerto debe ser 80 en este caso):
    ```bash
    ./web_downloader http://example.com:80/resource
    ```

## Notas

- Este programa solo soporta descargas en HTTP (puerto 80).
- Los recursos se guardarán en el sistema de archivos local, replicando la estructura de directorios del servidor web.
- Si un recurso ya ha sido procesado, no se descargará de nuevo para evitar duplicados.
- El programa ignora archivos con la extensión `.swf`.
- La descarga es recursiva para enlaces relativos encontrados en las páginas HTML descargadas.

## Código Fuente

El código fuente está en `web_downloader.c`. Aquí tienes una breve descripción de las funciones principales:

- `save_to_file`: Guarda datos en un archivo.
- `download_resource`: Descarga un recurso web.
- `is_empty`: Verifica si un archivo o directorio está vacío.
- `has_been_processed`: Verifica si un recurso ya ha sido procesado.
- `mark_as_processed`: Marca un recurso como procesado.
- `create_directory`: Crea un directorio en el sistema de archivos.
- `strcasestr`: Encuentra una subcadena insensible a mayúsculas y minúsculas.

### Descripción del flujo del programa

1. El programa recibe una URL como argumento.
2. Se analiza la URL para extraer la dirección, puerto y recurso.
3. Se establece una conexión con el servidor web.
4. Se envía una solicitud HTTP GET para descargar el recurso.
5. Se guarda el contenido descargado en el sistema de archivos local.
6. Si el contenido es HTML, se analizan los enlaces y se descargan recursivamente los recursos enlazados.
