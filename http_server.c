#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "include/sans.h"


void send_response(int client_sock, const char* path) {
    struct stat sb;

    if (stat(path, &sb) < 0 || !S_ISREG(sb.st_mode)) {
        const char* error_response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 46\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "\r\n"
            "<html><body><h1>404 Not Found</h1></body></html>";
        sans_send_pkt(client_sock, error_response, strlen(error_response));
        return;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        const char* error_response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 46\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "\r\n"
            "<html><body><h1>404 Not Found</h1></body></html>";
        sans_send_pkt(client_sock, error_response, strlen(error_response));
        return;
    }

    long file_length = sb.st_size;

    char header[1024];
    snprintf(header, 1024,
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: %ld\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "\r\n", file_length);

    sans_send_pkt(client_sock, header, strlen(header));

    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        sans_send_pkt(client_sock, buffer, bytes);
    }
    fclose(file);
}

int sanitize_path(const char* path) {
    int len = strlen(path);
    for (int i = 0; i < len - 1; i++) {
        if (path[i] == '.' && path[i + 1] == '.') {
            return 0;
        }
    }
    return 1;
}

int http_server(const char* iface, int port) {
    int client_sock = sans_accept(iface, port, IPPROTO_TCP);
    if (client_sock < 0) {
        return -1;
    }

    char buffer[1024];
    memset(buffer, 0, 1024);
    int len = sans_recv_pkt(client_sock, buffer, 1024 - 1);
    if (len <= 0) {
        sans_disconnect(client_sock);
        return -1;
    }

    buffer[len] = '\0';

    char method[8];
    char fpath[256];
    if (sscanf(buffer, "%7s %255s", method, fpath) != 2) {
        send_response(client_sock, NULL);
        sans_disconnect(client_sock);
        return -1;
    }

    if (strncmp(method, "GET", 3) != 0) {
        send_response(client_sock, NULL);
        sans_disconnect(client_sock);
        return -1;
    }

    char *path = fpath + 1;
    if (!sanitize_path(path) || strlen(path) == 0) {
        send_response(client_sock, NULL);
        sans_disconnect(client_sock);
        return -1;
    }

    send_response(client_sock, path);

    sans_disconnect(client_sock);
    return 0;
}





