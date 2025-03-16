#ifndef SCHEDULER_H

#define SCHEDULER_H

#include <stdint.h>

#include "../agent/agent.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define AGENTS_CONFIG_FILE "src/scheduler/agents.conf"

#define MAX_BUFFER_SIZE 4096

struct Scheduler {
    Agent* agents;
    uint8_t num_agents;

};

typedef struct Scheduler Scheduler;

Scheduler* scheduler_create(uint8_t num_agents);

void scheduler_destroy(Scheduler* scheduler);

void scheduler_run(Scheduler* scheduler);
#endif 