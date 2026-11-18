# Priority Scheduling: COMP 211 Checkoff 3

An exploration of **priority scheduling**: how an operating system decides which process gets the CPU when several are ready, what goes wrong when it only looks at priority, and how Linux actually handles it.

I learned this topic through a guided conversation with an AI tutor. I predicted results, got corrected, explained ideas back in my own words, and then wrote code and read primary sources to check the claims myself.

## What I Explored

- **Priority scheduling basics:** the scheduler runs the highest-priority *ready* process. For Unix `nice` values, a lower number means a higher priority.
- **Preemptive vs. non-preemptive:** whether a newly arrived, higher-priority process can kick the running process off the CPU.
- **Process states:** running, ready, and blocked. The scheduler only chooses among *ready* processes, so a blocked process can't run no matter how high its priority is.
- **Starvation and aging:** low-priority work can wait forever, and aging fixes this by improving a process's priority the longer it waits.
- **Priority inversion:** the bug that kept resetting the Mars Pathfinder lander in 1997, and the fix, priority inheritance.
- **How Linux really does it:** nice values are *weights* that set each process's share of the CPU, not strict priorities.

## Most Interesting Discoveries

### 1. Starvation isn't a bug. It's the design.
With strict priority, if high-priority work keeps arriving, a low-priority process never runs. The scheduler is doing exactly what it was designed to do. **Aging** fixes this: every X time units a process waits, its priority improves, so it eventually gets to run.

### 2. Priority inversion: medium tasks beat a high task on Mars
On Mars Pathfinder, a **low**-priority weather task (ASI/MET) held a mutex. A **high**-priority task (bc_dist) needed that mutex, so it was blocked. Then **several medium**-priority tasks preempted the low task. The low task couldn't finish and release the mutex, so the high task was stuck behind the medium tasks. When the highest-priority task (bc_sched) saw that bc_dist had missed its deadline, it reset the spacecraft.

The lock wasn't one the programmers wrote directly. It was a mutex hidden inside VxWorks' `select()` and pipe code, which they used for communication between tasks.

**Fix: priority inheritance.** While a low-priority task holds a lock that a high-priority task needs, it temporarily inherits the high priority. The medium tasks can't preempt it, so it finishes, releases the lock, and drops back to low. JPL turned this on for the mutex and uploaded the fix to Mars.

Downsides: a long critical section lets low-priority work run at high priority and push aside everything else, chains of locks get complicated, and it doesn't fix deadlock.

### 3. On Linux, "lowest priority" doesn't mean "never runs"
I predicted that a `nice 19` process competing with a `nice 0` process on one core would get **0%** of the CPU. It actually gets about **1.5%**. Linux's normal scheduler (CFS, and EEVDF since kernel 6.6) gives each nice value a **weight** (nice 0 = 1024, nice 5 = 335, nice 19 = 15), and every process gets CPU time in proportion to its weight. I confirmed these weights in the Linux kernel source. Strict priority does exist in Linux, but only for real-time policies (`SCHED_FIFO` / `SCHED_RR`, priorities 1–99, where higher means more important).

## Code

| File | What it does |
|---|---|
| `priority_sim.c` | Simulates priority scheduling one time unit at a time: non-preemptive, preemptive, and preemptive + aging. Prints a timeline. |
| `busy.c` | Spins on the CPU for N seconds and reports how much work it completed. |
| `run_nice.sh` | Runs two copies of `busy` on the same core (one at nice 0, one at nice N) and compares their CPU share. |

### How to Run (Linux or WSL)

```bash
# Simulator
gcc -Wall -o priority_sim priority_sim.c
./priority_sim            # both scenarios
./priority_sim basic      # A/B/C example only
./priority_sim starve     # starvation + aging only

# Nice experiment (requires Linux: uses taskset)
chmod +x run_nice.sh
./run_nice.sh             # nice 0 vs 19, 10 seconds
./run_nice.sh 5           # nice 0 vs 5
```

Notes:
- If you get `bad interpreter` on WSL, run `sed -i 's/\r$//' run_nice.sh` to fix the Windows line endings.
- Run `run_nice.sh` as one script. Starting the two programs from separate terminals can hide the nice effect because of Linux's autogroup feature.

## Results

### Scheduling simulator

```
== non-preemptive ==
Timeline: AAAAABBBCC
  A    finished at t=5   waited 0 units
  B    finished at t=8   waited 4 units
  C    finished at t=10  waited 6 units

== preemptive ==
Timeline: ABBBCCAAAA
  A    finished at t=10  waited 5 units
  B    finished at t=4   waited 0 units
  C    finished at t=6   waited 2 units

== preemptive (starvation scenario) ==
Timeline: HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
  A    never finished (STARVED, waited 60 units)

== preemptive + aging ==
Timeline: HHHHHHHHHHHHHHHHHHHHAAAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
  A    finished at t=24  waited 20 units
```

With aging, A's priority goes 3 → 2 at t=10 and 2 → 1 at t=20. At t=20 it ties with the arriving high-priority processes and wins the tie because it arrived first.

### Linux nice experiment

Two identical CPU-bound programs pinned to one core for 10 seconds (WSL 2):

| Nice values | Predicted CPU share | Measured CPU share |
|---|---|---|
| 0 vs 19 | 98.6% / 1.4% | 98.4% / 1.6% |
| 0 vs 5 | 75.3% / 24.7% | 75.3% / 24.7% |

The nice 0 vs 5 result matched the weight table exactly. The nice 19 result was slightly higher than predicted, probably because a process with so little weight gets tiny time slices, so small effects (scheduler timing, background processes) make up a bigger share of its CPU time.

Full output is in `results_simulator.txt`, `results_nice19.txt`, and `results_nice5.txt`.

## Takeaways

- The textbook model (strict priority) and what Linux does by default are different. "Priority" on Linux means "a bigger share," not "always goes first."
- Many scheduling bugs come from the interaction between scheduling and **locks**. Priority inversion only happens because of a shared mutex, and on Pathfinder that mutex was hidden inside OS code the programmers didn't write.
- Checking things myself mattered. The AI's first version of the Pathfinder story was simplified (one medium task, an obvious bus lock), and the firsthand account corrected it. Running the nice experiment turned "the AI says nice 19 gets about 1.4%" into something I measured.

## Questions I Still Have

- How does EEVDF decide *when* each process runs, not just how much CPU it gets?
- How do real-time systems prove that a high-priority task will always meet its deadline?

## Presentation

Slides: `slides/` *(add link or file)*

## Resources

- Glenn Reeves (Pathfinder software lead), "What really happened on Mars?": https://www.cs.unc.edu/~anderson/teach/comp790/papers/mars_pathfinder_long_version.html
- Mike Jones, "What really happened on Mars Rover Pathfinder": https://www.cs.cornell.edu/courses/cs614/1999sp/papers/pathfinder.html
- Linux kernel source, `sched_prio_to_weight` table: https://github.com/torvalds/linux/blob/master/kernel/sched/core.c
