/*
 * priority_sim.c - simulate priority scheduling one time unit at a time.
 *
 * Lower priority number = higher priority.
 * Ties are broken by arrival time (first come, first served).
 * Preemption only happens when a ready process is STRICTLY better.
 *
 * Build: gcc -Wall -o priority_sim priority_sim.c
 * Run:   ./priority_sim            (both scenarios)
 *        ./priority_sim basic      (A/B/C example)
 *        ./priority_sim starve     (starvation + aging)
 */
#include <stdio.h>
#include <string.h>

#define MAX_PROCS 64
#define MAX_TIME 200
#define AGE_INTERVAL 10 /* every 10 units waiting, priority number drops by 1 */

typedef enum { NON_PREEMPTIVE, PREEMPTIVE, PREEMPTIVE_AGING } Mode;

typedef struct {
    char name[8];
    int arrival;
    int burst;
    int priority;  /* base priority */
    int remaining;
    int waited;    /* time spent ready but not running */
    int finish;    /* -1 = never finished */
} Process;

static const char *mode_name(Mode m) {
    switch (m) {
    case NON_PREEMPTIVE: return "non-preemptive";
    case PREEMPTIVE: return "preemptive";
    default: return "preemptive + aging";
    }
}

static int effective_priority(const Process *p, Mode m) {
    if (m != PREEMPTIVE_AGING) return p->priority;
    int eff = p->priority - p->waited / AGE_INTERVAL;
    return eff < 0 ? 0 : eff;
}

static int is_ready(const Process *p, int t) {
    return p->arrival <= t && p->remaining > 0;
}

/* Returns 1 if a should be chosen over b. */
static int better(const Process *a, const Process *b, Mode m) {
    int pa = effective_priority(a, m), pb = effective_priority(b, m);
    if (pa != pb) return pa < pb;
    return a->arrival < b->arrival;
}

static void simulate(Process *procs, int n, Mode m, int end_time, int verbose) {
    char timeline[MAX_TIME + 1];
    int current = -1;

    for (int i = 0; i < n; i++) {
        procs[i].remaining = procs[i].burst;
        procs[i].waited = 0;
        procs[i].finish = -1;
    }

    for (int t = 0; t < end_time; t++) {
        int best = -1;
        for (int i = 0; i < n; i++)
            if (is_ready(&procs[i], t) && (best == -1 || better(&procs[i], &procs[best], m)))
                best = i;

        if (current != -1 && procs[current].remaining > 0) {
            int keep = (m == NON_PREEMPTIVE) ||
                       effective_priority(&procs[best], m) >= effective_priority(&procs[current], m);
            if (keep) best = current;
        }
        current = best;

        if (current == -1) {
            timeline[t] = '.';
            continue;
        }

        timeline[t] = procs[current].name[0];
        procs[current].remaining--;
        if (procs[current].remaining == 0) procs[current].finish = t + 1;

        for (int i = 0; i < n; i++)
            if (i != current && is_ready(&procs[i], t)) procs[i].waited++;
    }
    timeline[end_time] = '\0';

    printf("\n== %s ==\n", mode_name(m));
    printf("Timeline: %s\n", timeline);
    for (int i = 0; i < n; i++) {
        if (!verbose && procs[i].name[0] != 'A') continue;
        if (procs[i].finish == -1)
            printf("  %-4s never finished (STARVED, waited %d units)\n", procs[i].name, procs[i].waited);
        else
            printf("  %-4s finished at t=%-3d waited %d units\n", procs[i].name, procs[i].finish, procs[i].waited);
    }
}

static void basic_scenario(void) {
    Process procs[] = {
        {"A", 0, 5, 3}, {"B", 1, 3, 1}, {"C", 2, 2, 2},
    };
    int n = sizeof procs / sizeof procs[0];
    printf("\n######## Scenario 1: A/B/C (lower number = higher priority) ########\n");
    printf("  A: arrives 0, burst 5, priority 3\n");
    printf("  B: arrives 1, burst 3, priority 1\n");
    printf("  C: arrives 2, burst 2, priority 2\n");
    simulate(procs, n, NON_PREEMPTIVE, 10, 1);
    simulate(procs, n, PREEMPTIVE, 10, 1);
}

static void starvation_scenario(void) {
    Process procs[MAX_PROCS];
    int n = 0, end_time = 60;

    procs[n++] = (Process){"A", 0, 4, 3};
    for (int t = 0; t < end_time && n < MAX_PROCS; t += 2) {
        procs[n] = (Process){"H", t, 2, 1};
        snprintf(procs[n].name, sizeof procs[n].name, "H%d", n);
        n++;
    }

    printf("\n######## Scenario 2: starvation ########\n");
    printf("  A: arrives 0, burst 4, priority 3\n");
    printf("  H: a new priority-1 process (burst 2) arrives every 2 units, forever\n");
    printf("  Aging: priority number drops by 1 every %d units waiting\n", AGE_INTERVAL);
    simulate(procs, n, PREEMPTIVE, end_time, 0);
    simulate(procs, n, PREEMPTIVE_AGING, end_time, 0);
}

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "basic") == 0) basic_scenario();
    if (argc < 2 || strcmp(argv[1], "starve") == 0) starvation_scenario();
    return 0;
}
