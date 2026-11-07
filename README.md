*This project has been created as part of the 42 curriculum by nrajaoar.*

# Codexion

> Master the race for resources before the deadline masters you.

## Description

**Codexion** is a concurrency simulation written in C with POSIX threads. A group of coders sits around a circular, inclusive co-working table. In the middle stands a single shared **Quantum Compiler**. To use it, a coder must hold **two USB dongles at the same time**: one on their left and one on their right.

Each coder loops through three mutually exclusive activities:

1. **Compile**: requires both adjacent dongles for `time_to_compile` ms.
2. **Debug**: lasts `time_to_debug` ms, with no dongle held.
3. **Refactor**: lasts `time_to_refactor` ms, with no dongle held. Right after it, the coder tries to compile again.

There are **as many dongles as coders**, one between each pair of neighbours (with a single coder, there is a single dongle). Coders do not communicate with each other and cannot tell whether a colleague is about to burn out. If a coder does not **start** a compile within `time_to_burnout` ms of the start of their previous compile (or of the simulation), they **burn out** and the simulation stops. It also stops once every coder has compiled at least `number_of_compiles_required` times.

The goal is to orchestrate this race without deadlock, without starvation and without data races, while respecting:

- a **cooldown** on every dongle after it is released,
- a **scheduling policy** (`fifo` or `edf`) used by each dongle to arbitrate between competing coders,
- a **precise burnout detection** (reported within 10 ms of the real burnout),
- **serialized logs** that never interleave.

### Seating

Coder `1` sits next to coder `number_of_coders`. Any other coder `N` sits between coder `N-1` and coder `N+1`. Coder `i` (0-based index) uses dongle `i` on their left and dongle `(i + 1) % number_of_coders` on their right.

```
 (dongle 1)  coder 1  (dongle 2)  coder 2  (dongle 3)  coder 3  ...
```

Dongle 2 is shared by coder 1 and coder 2.

### Project structure

```
.
├── Makefile
├── README.md
└── coders/
    ├── codexion.h      structures and prototypes
    ├── main.c          entry point
    ├── parsing.c       argument validation
    ├── simulation.c    thread creation, cleanup
    ├── coder.c         coder life cycle (compile / debug / refactor)
    ├── monitor.c       burnout and completion detection
    ├── dongle.c        dongle init / destroy / release
    ├── dongle_take.c   dongle acquisition (queue + cooldown)
    ├── heap.c          priority queue (push, pop, comparison)
    ├── heap_utils.c    priority queue (sift, peek, remove)
    ├── log.c           serialized output, stop flag
    └── time.c          time helpers
```

No global variable is used: everything lives in the `t_env` structure passed to every thread.

## Instructions

### Compilation

```bash
make        # builds ./codexion with cc -Wall -Wextra -Werror -pthread
make clean  # removes object files
make fclean # removes object files and the binary
make re     # fclean + all
```

### Execution

```bash
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
           time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

All eight arguments are mandatory.

| Argument | Unit | Meaning |
|---|---|---|
| `number_of_coders` | – | Number of coders (and of dongles). Must be ≥ 1. |
| `time_to_burnout` | ms | Maximum delay between the start of two compiles (or the simulation start and the first compile). |
| `time_to_compile` | ms | Duration of a compile; two dongles are held during that time. |
| `time_to_debug` | ms | Duration of the debug phase. |
| `time_to_refactor` | ms | Duration of the refactor phase. |
| `number_of_compiles_required` | – | The simulation stops once every coder has compiled at least this many times. |
| `dongle_cooldown` | ms | Time during which a released dongle cannot be taken again. |
| `scheduler` | – | `fifo` (first in, first out) or `edf` (earliest deadline first, `deadline = last_compile_start + time_to_burnout`). |

Invalid inputs (negative numbers, non-integers, values above `INT_MAX`, wrong argument count, unknown scheduler) are rejected with `Error: Invalid arguments` on `stderr` and exit code `1`.

### Examples

```bash
./codexion 5 800 200 200 200 5 0 fifo     # runs until each coder compiled 5 times
./codexion 5 800 200 200 200 5 50 edf     # same with a 50 ms dongle cooldown
./codexion 4 410 200 200 200 5 0 edf      # too tight: a coder burns out
./codexion 1 800 200 200 200 3 0 fifo     # one dongle only: burnout is unavoidable
```

### Output format

Each state change prints one line, `timestamp_in_ms X message`, where `timestamp_in_ms` is the time elapsed since the start of the simulation and `X` is the coder number:

```
0 1 has taken a dongle
1 1 has taken a dongle
1 1 is compiling
201 1 is debugging
401 1 is refactoring
...
1204 3 burned out
```

## Technical choices

- **One priority queue (binary heap) per dongle.** Every coder wanting a dongle pushes a request `(coder_id, deadline, req_seq)` into that dongle's heap. The comparison function depends on the scheduler:
  - `fifo`: ordered by `req_seq`, a global arrival counter.
  - `edf`: ordered by `deadline`, then by `req_seq`, then by `coder_id`.

  Only the coder at the top of the heap is allowed to take the dongle.
- **Time** is measured with `gettimeofday()` in milliseconds. Sleeps are done by `ft_usleep`, which sleeps in 500 µs slices so it can be interrupted as soon as the simulation ends.
- **One monitor thread** runs alongside the coder threads and is the only one in charge of burnout detection and of the "everyone has compiled enough" condition.

## Blocking cases handled

### Deadlock prevention (Coffman's conditions)

A deadlock requires all four Coffman conditions to hold at once. Three of them are inherent to the problem; the fourth is broken on purpose.

| Condition | Status in Codexion |
|---|---|
| Mutual exclusion | Present: a dongle has one owner at a time (`owner` field protected by the dongle mutex). |
| Hold and wait | Present: a coder holds their first dongle while waiting for the second. |
| No preemption | Present: a dongle is never taken away from its owner. |
| **Circular wait** | **Broken**: dongles are always acquired in a global order (resource hierarchy). |

In `set_order`, each coder takes the dongle with the **lowest index first**, then the other one. Since every coder follows the same global order, a cycle of "A waits for B who waits for C who waits for A" cannot form. If a coder obtains the first dongle but cannot obtain the second (the simulation ends), they release the first one before returning.

Single-coder case: there is only one dongle, so first and second dongle are the same. The coder takes it, can never take "the second one", and burns out after `time_to_burnout`. The burnout is reported normally and nothing blocks.

### Starvation prevention

- Each dongle serves its waiting coders through its **priority queue**, so the order is never left to the OS scheduler or to a lucky mutex acquisition.
- Under `fifo`, requests are served strictly in arrival order (`req_seq`).
- Under `edf`, the coder closest to burnout is served first, which is exactly what keeps a starved coder alive. Ties are broken deterministically by `req_seq` and then `coder_id`.
- Because the head of the queue is the only coder allowed to take the dongle, a coder cannot be overtaken indefinitely by a faster neighbour. Liveness holds as long as the parameters are feasible.

### Cooldown handling

When a dongle is released, `cooldown_until = now + dongle_cooldown` is stored in it. A coder at the head of the queue who finds the dongle free but still cooling down does not spin: it sleeps with `pthread_cond_timedwait` on the absolute cooldown deadline and retries when it wakes up. Coders not at the head wait on the condition variable and are woken by the next release.

### Precise burnout detection

The monitor thread loops every 500 µs. Under `state_mutex`, it checks for each coder whether `now - last_compile_start > time_to_burnout`. `last_compile_start` is updated under the same mutex the instant a compile begins, so the monitor never sees a half-updated value. In practice a burnout is reported about 1 ms after it occurs, well within the 10 ms requirement.

`print_burnout` sets the `is_over` flag and prints the message **in the same critical section**, so exactly one burnout line is ever printed and no other message can follow it.

### Log serialization

Every message goes through `print_status` or `print_burnout`, which hold `log_mutex` for the whole `printf`. Inside, `state_mutex` is taken to check `is_over`, so nothing is printed once the simulation has finished. Lines therefore never interleave, and no state message appears after a burnout.

### Clean termination

When the simulation ends (burnout or completion), `stop_simulation` sets `is_over` and calls `wake_all`, which broadcasts the condition variable of every dongle. Coders blocked in `pthread_cond_wait` / `pthread_cond_timedwait` wake up, notice the flag, remove their pending request from the heap (`pq_remove`) and exit. `ft_usleep` also checks the flag, so sleeping coders stop within 500 µs. Finally all threads are joined and all heap, dongle and coder memory is freed.

## Thread synchronization mechanisms

### Primitives used

| Primitive | Where | Protects / role |
|---|---|---|
| `pthread_t` (one per coder + one monitor) | `simulation.c` | Each coder is a thread created with `pthread_create`; the monitor is a separate thread. All are joined in `cleanup_env`. |
| `pthread_mutex_t mutex` (per dongle) | `t_dongle` | The dongle's state: `owner`, `cooldown_until` and its priority queue. |
| `pthread_cond_t cond` (per dongle) | `t_dongle` | Waiting queue of the dongle. Coders sleep on it until a release, a cooldown expiry or the end of the simulation. |
| `pthread_mutex_t state_mutex` | `t_env` | Shared simulation state: `is_over`, `seq_counter`, each coder's `last_compile_start` and `compiles_count`. |
| `pthread_mutex_t log_mutex` | `t_env` | Standard output, so one message is written at a time. |

No custom event system is used: the condition variables and the `is_over` flag are enough.

### How a coder takes a dongle

```
take_dongle():
    lock(dongle.mutex)
    push request (id, deadline, seq) in dongle.pq        # seq/deadline read under state_mutex
    while not simulation_over:
        if dongle free AND I am the head of the queue:
            if cooldown elapsed: take it, pop my request, break
            else: pthread_cond_timedwait(until cooldown_until)
        else:
            pthread_cond_wait(dongle.cond)
    unlock(dongle.mutex)
    print "has taken a dongle"
```

`release_dongle` locks the dongle, sets `owner = -1`, stores the new `cooldown_until` and calls `pthread_cond_broadcast` so every waiter re-evaluates the situation.

### Lock ordering

To rule out deadlocks between the mutexes themselves, locks are always nested in the same direction and never in reverse:

- `dongle.mutex` → `state_mutex` (when enqueuing a request, and when checking `is_over`),
- `log_mutex` → `state_mutex` (when printing),
- `state_mutex` is never held while locking a dongle mutex or `log_mutex`. `wake_all` is called after `state_mutex` is released.

The dongle mutex is released before printing, so a slow `printf` never blocks other coders from taking dongles.

### Race conditions prevented (examples)

- **Dongle duplication.** The test "is the dongle free?" and the assignment `owner = my_id` happen in the same critical section under the dongle mutex, so two coders can never both believe they own it.
- **Lost wake-up at shutdown.** `is_over` is set under `state_mutex` *before* `wake_all` locks each dongle mutex. A coder is therefore either already waiting on the condition variable (and gets the broadcast) or has not yet evaluated its loop condition (and sees `is_over = 1`). It can never go to sleep after the last broadcast.
- **Monitor vs coder data.** `last_compile_start` and `compiles_count` are written by coders and read by the monitor, always under `state_mutex`, so the monitor never decides on a torn or stale value.
- **Request ordering.** `seq_counter` is incremented under `state_mutex`, giving each request a unique, totally ordered sequence number that fifo and edf tie-breaks rely on.
- **Double burnout / message after burnout.** The check of `is_over` and its update to `1` are done in a single critical section in `print_burnout`, so only the first burnout is printed and every later message is suppressed.

### Thread-safe communication between coders and the monitor

Coders and the monitor never exchange messages directly. They communicate only through the shared `t_env`, always under `state_mutex`:

- coders **publish** their progress (`last_compile_start`, `compiles_count`);
- the monitor **reads** it and, when needed, **raises** `is_over`;
- coders **read** `is_over` (`simulation_over`) in their loops and sleeps, and the broadcast from `wake_all` brings blocked ones back to check it.

## Resources

### References

- POSIX Threads manual pages: `pthread_create(3)`, `pthread_mutex_lock(3)`, `pthread_cond_wait(3)`, `pthread_cond_timedwait(3)`, `pthread_cond_broadcast(3)`, `gettimeofday(2)`, `usleep(3)`.
- E. G. Coffman, M. J. Elphick, A. Shoshani, *System Deadlocks*, ACM Computing Surveys, 1971 (the four deadlock conditions).
- E. W. Dijkstra, *Hierarchical ordering of sequential processes*, 1971 (dining philosophers and resource ordering).
- C. L. Liu, J. W. Layland, *Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment*, 1973 (Earliest Deadline First).
- T. H. Cormen et al., *Introduction to Algorithms*, chapter on heaps and priority queues.
- A. Downey, *The Little Book of Semaphores* (classic synchronization problems).

### Use of AI

AI tools were used as a support, not as a code generator:

- to review the final code and discuss possible concurrency pitfalls (lost wake-ups, lock ordering, shutdown handling);
- to help test the program with a variety of parameters and to double-check the behaviour against the subject;

All the code was written, reviewed and tested by the author, who can explain every part of it.