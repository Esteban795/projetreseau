#ifndef AGENT_H

#define AGENT_H

#include <stdint.h>
#define MAX_AGENTS 10

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFFER_SIZE 4096

#define MAX_COMMANDS_LENGTH 100


struct Agent {
    char commands[256];
    char ip[16];
    int port;
};

typedef struct Agent Agent;

#endif