#include "../../include/scheduler/scheduler.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>


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
    printf("[Agent] : %hhu.%hhu.%hhu.%hhu:%hu\n", agent->ip[0], agent->ip[1], agent->ip[2], agent->ip[3], agent->port);

    // Create TCP socket

    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    if (sockfd < 0) {
        printf("[Agent] : Error creating socket\n");
        return NULL;
    }

    struct sockaddr_in agent_addr;
    memset(&agent_addr, 0, sizeof(agent_addr));
    agent_addr.sin_family = AF_INET;
    agent_addr.sin_port = htons(agent->port);
    agent_addr.sin_addr.s_addr = inet_addr(agent->ip);

    if (connect(sockfd, (struct sockaddr*)&agent_addr, sizeof(agent_addr)) < 0) {
        printf("[Agent] : Error connecting to agent\n");
        close(sockfd); // Necessary, because if it fails, the socket is not closed until a few minutes by the OS
        return NULL;
    }

    printf("[Agent] : Connected to agent %s:%hu\n", agent->ip, agent->port);

    ssize_t bytes_sent = write(sockfd,agent->commands,strlen(agent->commands));

    if (bytes_sent < 0) {
        printf("[Agent] : Error sending commands\n");
        close(sockfd);
        return NULL;
    }

    printf("[Agent] : Sent %ld bytes\n", bytes_sent);
    printf("[Scheduler] Command '%s' sent to agent %s:%hu\n", agent->commands, agent->ip, agent->port);
    
    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, MAX_BUFFER_SIZE);

    printf("[Scheduler] : Waiting for response from agent %s:%hu\n", agent->ip, agent->port);

    ssize_t bytes_rcved;
    while ((bytes_rcved = read(sockfd, buffer, MAX_BUFFER_SIZE)) > 0) {
        buffer[bytes_rcved] = '\0'; // Force to null-terminate the string
        printf("[Agent] : Received %ld bytes\n", bytes_rcved);
        printf("[Agent] : %s\n", buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    printf("[Scheduler] : Agent %hhu.%hhu.%hhu.%hhu:%hu finished\n", agent->ip[0], agent->ip[1], agent->ip[2], agent->ip[3], agent->port);
    close(sockfd);
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


