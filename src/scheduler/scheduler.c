#include "../../include/scheduler/scheduler.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

SSL_CTX* create_ssl_context() {
    const SSL_METHOD* method;
    SSL_CTX* ctx;

    OpenSSL_add_ssl_algorithms();
    method = SSLv23_client_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

Scheduler* scheduler_create(uint8_t num_agents) {
    Scheduler* scheduler = malloc(sizeof(Scheduler));
    scheduler->agents = NULL;
    scheduler->num_agents = num_agents;
    return scheduler;
}

void scheduler_init(Scheduler* scheduler) {
    FILE* file = fopen(AGENTS_CONFIG_FILE, "r");

    if (file == NULL) {
        printf("Error opening file %s\n", AGENTS_CONFIG_FILE);
        exit(1);
    }
    scheduler->agents = malloc(sizeof(Agent) * scheduler->num_agents);
    int i = 0;
    printf("[Scheduler] num_agents = %d\n", scheduler->num_agents);
    // Scans each line formatted such as IP_ADDR PORT "command"
    while (fscanf(file, "%15s %d \"%255[^\"]\"\n",  scheduler->agents[i].ip, &scheduler->agents[i].port, scheduler->agents[i].commands) == 3) {
        printf("[Scheduler] : agent %s:%hu\n", scheduler->agents[i].ip, scheduler->agents[i].port);
        i++;
        if (i >= scheduler->num_agents) { // We have read all agents
            break;
        }
    }

    fclose(file);

    printf("[Scheduler] : found %d agents\n", i);

    scheduler->num_agents = i;
}

void scheduler_destroy(Scheduler* scheduler) {
    free(scheduler->agents);
    free(scheduler);
}

void* agent_run(void* agent_arg) {
    Agent* agent = (Agent*) agent_arg;
    printf("[Agent] : %s:%hu\n", agent->ip, agent->port);

    // Create TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("[Agent] : Error creating socket\n");
        return NULL;
    }

    // Init SSL context.
    // Don't ask me how it works, I took it from openssl tutorial
    SSL_CTX* ctx = create_ssl_context();
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);

    struct sockaddr_in agent_addr;
    memset(&agent_addr, 0, sizeof(agent_addr));
    agent_addr.sin_family = AF_INET;
    agent_addr.sin_port = htons(agent->port);
    agent_addr.sin_addr.s_addr = inet_addr(agent->ip);

    if (connect(sockfd, (struct sockaddr*)&agent_addr, sizeof(agent_addr)) < 0) {
        printf("[Agent] : Error connecting to agent\n");
        close(sockfd);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        close(sockfd);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        return NULL;
    }

    printf("[Agent] : Connected to agent %s:%hu\n", agent->ip, agent->port);

    int bytes_sent = SSL_write(ssl, agent->commands, strlen(agent->commands));
    if (bytes_sent <= 0) {
        printf("[Agent] : Error sending commands\n");
        ERR_print_errors_fp(stderr);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sockfd);
        SSL_CTX_free(ctx);
        return NULL;
    }

    printf("[Agent] : Sent %d bytes\n", bytes_sent);
    printf("[Scheduler] Command '%s' sent to agent %s:%hu\n", agent->commands, agent->ip, agent->port);

    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, MAX_BUFFER_SIZE);

    printf("[Scheduler] : Waiting for response from agent %s:%hu\n", agent->ip, agent->port);

    ssize_t bytes_rcved;
    while ((bytes_rcved = SSL_read(ssl, buffer, MAX_BUFFER_SIZE)) > 0) {
        buffer[bytes_rcved] = '\0'; // Force to null-terminate the string
        printf("[Agent] : Received %ld bytes\n", bytes_rcved);
        printf("[Agent] : %s\n", buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    if (bytes_rcved < 0) {
        printf("[Agent] : Error receiving response\n");
        ERR_print_errors_fp(stderr);
    }

    printf("[Scheduler] : Agent %s:%hu finished\n", agent->ip, agent->port);

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sockfd);
    SSL_CTX_free(ctx);

    return NULL;
}

void scheduler_run(Scheduler* scheduler) {
    scheduler_init(scheduler);

    pthread_t* threads = malloc(sizeof(pthread_t) * scheduler->num_agents);
    for (int i = 0; i < scheduler->num_agents; i++) {
        pthread_create(&threads[i], NULL, agent_run, &scheduler->agents[i]);
    }

    for (int i = 0; i < scheduler->num_agents; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    printf("[Scheduler] : all agents finished\n");
}


