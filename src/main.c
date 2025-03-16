#include <stdio.h>
#include "../include/scheduler/scheduler.h"

int main(int argc, const char * argv[]) {
    if (argc < 2) {
        printf("Usage: %s <num_agents>\n", argv[0]);
        return 1;
    }
    Scheduler* scheduler = scheduler_create(atoi(argv[1]));
    scheduler_run(scheduler);
    scheduler_destroy(scheduler);
    return 0;
}