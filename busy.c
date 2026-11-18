/*
 * busy.c - spin on the CPU for N seconds and report how much work got done.
 *
 * Build: gcc -O2 -Wall -o busy busy.c
 * Run:   ./busy 10
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(int argc, char **argv) {
    int seconds = argc > 1 ? atoi(argv[1]) : 10;
    double end = now() + seconds;
    volatile unsigned long work = 0;
    unsigned long chunks = 0;

    while (now() < end) {
        for (int i = 0; i < 1000000; i++) work++;
        chunks++;
    }

    printf("pid %d: %lu million iterations\n", getpid(), chunks);
    return 0;
}
