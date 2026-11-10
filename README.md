*This project has been created as part of the 42 curriculum by nrajaoar.*

# Codexion

> Master the race for resources before the deadline masters you.

## Description

**Codexion** is a concurrency simulation written in C with POSIX threads. A group of coders sits around a circular, inclusive co-working table. In the middle stands a single shared **Quantum Compiler**. To use it, a coder must hold **two USB dongles at the same time**: one on their left and one on their right.

Each coder loops through three mutually exclusive activities:

1. **Compile**: requires both adjacent dongles for `time_to_compile` ms.
2. **Debug**: lasts `time_to_debug` ms, with no dongle held.
3. **Refactor**: lasts `time_to_refactor` ms, with no dongle held. Right after it, the coder tries to compile again.

The dongles are given back **as soon as the compile ends**, so debugging and refactoring are the time during which the neighbours can use them.

There are **as many dongles as coders**, one between each pair of neighbours (with a single coder, there is a single dongle). Coders do not communicate with each other and cannot tell whether a colleague is about to burn out. If a coder does not **start** a compile within `time_to_burnout` ms of the start of their previous compile (or of the simulation), they **burn out** and the simulation stops. It also stops once every coder has compiled `number_of_compiles_required` times.

The goal is to orchestrate this race without deadlock, without starvation and without data races, while respecting:

- a **cooldown** on every dongle after it is released,
- a **scheduling policy** (`fifo` or `edf`) used by the dongles to arbitrate between competing coders,
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
    ├── codexion.h       structures and prototypes
    ├── main.c           entry point
    ├── parsing.c        argument validation
    ├── simulation.c     thread creation, cleanup
    ├── coder.c          coder life cycle, compile counting and end of simulation
    ├── monitor.c        burnout and completion detection
    ├── dongle.c         dongle init / destroy / release
    ├── dongle_pair.c    acquisition of the two dongles (request in both queues)
    ├── dongle_wait.c    "can I take both now?" decision, waiting
    ├── dongle_take.c    acquisition of a single dongle (one-coder case only)
    ├── heap.c           priority queue (push, pop, comparison)
    ├── heap_utils.c     priority queue (sift, peek, remove)
    ├── log.c            serialized output, stop flags
    └── time.c           time helpers and sleeps
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
| `number_of_compiles_required` | – | Every coder compiles exactly this many times, then the simulation ends (see "End of the simulation"). With `0` the simulation ends immediately and prints nothing. |
| `dongle_cooldown` | ms | Time during which a released dongle cannot be taken again. |
| `scheduler` | – | `fifo` (first in, first out) or `edf` (earliest deadline first, `deadline = last_compile_start + time_to_burnout`). |

Invalid inputs (negative numbers, non-integers, values above `INT_MAX`, wrong argument count, unknown scheduler) are rejected with `Error: Invalid arguments` on `stderr` and exit code `1`.

### Examples

```bash
./codexion 5 800 200 200 200 5 0 fifo     # 25 compiles (5 x 5), no burnout
./codexion 5 800 200 200 200 5 50 edf     # same with a 50 ms dongle cooldown
./codexion 4 410 200 200 200 5 0 edf      # too tight: a coder burns out
./codexion 1 800 200 200 200 3 0 fifo     # one dongle only: burnout is unavoidable
./codexion 50 3000 200 200 200 15 800 edf # 50 coders, long cooldown: 750 compiles
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

### Choosing feasible parameters

Two conditions must hold for coders to survive. They are rules of thumb; keep a 5 to 10 % margin, because results at the limit depend on the OS scheduler.

1. **Life cycle**: `time_to_burnout > time_to_compile + time_to_debug + time_to_refactor`. Otherwise a coder burns out whatever the code does.
2. **Sharing the dongles**: a coder needs both neighbouring dongles, so two neighbours can never compile together. With an even number of coders at most half of them compile at the same time (2 "rounds" are needed so that everybody compiles once); with an odd number, one coder is left over, so 3 rounds are needed. A round lasts `time_to_compile + dongle_cooldown`, hence:

   `time_to_burnout > rounds * (time_to_compile + dongle_cooldown)`

   With a single coder there is only one dongle: burnout is certain.

Example: `./codexion 5 400 100 100 100 10 0 fifo` (cycle 300 ms < 400, 3 rounds x 100 ms = 300 ms < 400) runs without burnout; the coders compile in 3 rounds repeating every 300 ms.

## Technical choices

- **One priority queue (binary heap) per dongle.** A coder that wants to compile pushes **one request** `(coder_id, deadline, req_seq)` into the heap of **each** of its two dongles (same values in both). The comparison function depends on the scheduler:
  - `fifo`: ordered by `req_seq`, a global arrival counter.
  - `edf`: ordered by `deadline`, then by `req_seq`, then by `coder_id`.

  Each heap has a capacity of 2: a dongle has only two neighbours and each coder has at most one pending request.
- **The two dongles are taken together.** A coder never holds one dongle while waiting for the other (see "Deadlock prevention"). It locks both dongle mutexes (always the lowest index first), checks that both are free and cooled down, and takes them in the same critical section.
- **Priority rule.** The head of a dongle's heap is the most urgent coder waiting for it. A coder may take its two dongles if, for each of them, either it is the head, **or the head cannot run anyway** because the head's other dongle is currently held by someone. This keeps the scheduler fair (the most urgent coder is never overtaken by someone who would actually delay it) while not letting a blocked coder block its neighbours (see "Starvation prevention").
- **Time** is measured with `gettimeofday()` in milliseconds. `ft_usleep` (debug and refactor) and `compile_sleep` (compile) sleep in 500 µs slices.
- **One monitor thread** runs alongside the coder threads and is in charge of burnout detection.

### End of the simulation

- A compile is counted when it **starts**, at the very moment `is compiling` is printed (counter update and print are one critical section).
- Each coder stops after exactly `number_of_compiles_required` compiles, so the total number of `is compiling` lines is `number_of_coders * number_of_compiles_required` (for example 5 x 7 = 35). This satisfies the subject's "at least".
- When the last coder starts its last compile, the simulation is flagged as over **in the same critical section**: nothing is printed afterwards, so the last line of the log is always an `is compiling` line.
- The compiles in progress are **not cut**: they sleep for their full `time_to_compile`, release their dongles, and the program exits right after. Only a burnout (or an initialisation failure) interrupts them immediately, through a separate `aborted` flag.
- A coder who has reached their quota is ignored by the burnout check, since they do not need to compile again.

## Blocking cases handled

### Deadlock prevention (Coffman's conditions)

A deadlock requires all four Coffman conditions to hold at once.

| Condition | Status in Codexion |
|---|---|
| Mutual exclusion | Present: a dongle has one owner at a time (`owner` field protected by the dongle mutex). |
| Hold and wait | **Broken**: a coder never holds a dongle while waiting for another. Both are taken in one atomic step or none. |
| No preemption | Present: a dongle is never taken away from its owner. |
| Circular wait | **Broken** as well: the two dongle mutexes are always locked in a global order (lowest index first), and a coder that holds dongles is compiling, so it always releases them within `time_to_compile`. |

Breaking hold-and-wait is also what keeps the throughput high. A first version took the first dongle and then waited for the second: with many coders, almost everybody grabbed one dongle at the same time and blocked their neighbour, and only 1 to 11 of 50 coders could compile per round, which led to burnouts on feasible parameters (e.g. 50 coders, `time_to_burnout` 3000, `dongle_cooldown` 800). With atomic acquisition, half of the coders compile in every round, as expected.

Single-coder case: there is only one dongle, so the first and second dongle are the same. This is the only place where `take_dongle` is used: the coder takes the dongle, can never take "the second one", and burns out after `time_to_burnout`. The burnout is reported normally and nothing blocks.

### Starvation prevention

- Each dongle serves its waiting coders through its **priority queue**, so the order is never left to the OS scheduler or to a lucky mutex acquisition.
- Under `fifo`, requests are served in arrival order (`req_seq`).
- Under `edf`, the coder closest to burnout is served first. Ties are broken deterministically by `req_seq` and then `coder_id`.
- **The most urgent request always makes progress**: it is the head of both its queues, so nobody has priority over it, and it only waits for a dongle to be released or to finish its cooldown. Both always happen in bounded time. Liveness therefore holds as long as the parameters are feasible.
- **A blocked coder does not block the others.** If the head of a queue cannot run because its other dongle is busy, the next coder is allowed to use the dongle. Without this rule, requests ordered along the circle serialize everybody behind the first coder (one compile per round instead of half of the coders).

### Cooldown handling

When a dongle is released, `cooldown_until = now + dongle_cooldown` is stored in it. A coder that finds a dongle free but still cooling down does not spin: it sleeps with `pthread_cond_timedwait` on the absolute cooldown deadline and re-evaluates when it wakes up. A coder blocked by a busy dongle waits on its condition variable and is woken by the release. A coder that has to give way to a more urgent neighbour re-checks after 1 ms at most.

### Precise burnout detection

The monitor thread loops every 500 µs. Under `state_mutex`, it checks for each coder (still compiling) whether `now - last_compile_start > time_to_burnout`. `last_compile_start` is updated under the same mutex the instant a compile begins, so the monitor never sees a half-updated value. In practice a burnout is reported 1 to 3 ms after it occurs, well within the 10 ms requirement.

`print_burnout` sets the `is_over` and `aborted` flags and prints the message **in the same critical section**, so exactly one burnout line is ever printed and no other message can follow it.

### Log serialization

Every message goes through `print_status`, `print_burnout` or the compile announcement, which hold `log_mutex` for the whole `printf`. Inside, `state_mutex` is taken to check `is_over`, so nothing is printed once the simulation has finished. Lines therefore never interleave, and no state message appears after a burnout or after the last compile starts.

### Clean termination

When the simulation ends, the flag `is_over` is set and `wake_all` broadcasts the condition variable of every dongle. Coders blocked in `pthread_cond_wait` / `pthread_cond_timedwait` wake up, notice the flag, remove their pending requests from the heaps (`pq_remove`) and exit. `ft_usleep` also checks the flag, so a coder that is debugging or refactoring stops within 500 µs. Compiles in progress finish normally unless the end is a burnout (see "End of the simulation"). Finally all threads are joined and all heap, dongle and coder memory is freed (checked with AddressSanitizer).

## Thread synchronization mechanisms

### Primitives used

| Primitive | Where | Protects / role |
|---|---|---|
| `pthread_t` (one per coder + one monitor) | `simulation.c` | Each coder is a thread created with `pthread_create`; the monitor is a separate thread. All are joined in `cleanup_env`. |
| `pthread_mutex_t mutex` (per dongle) | `t_dongle` | The dongle's state: `owner`, `cooldown_until` and its priority queue. |
| `pthread_cond_t cond` (per dongle) | `t_dongle` | Waiting queue of the dongle. Coders sleep on it until a release, a cooldown expiry, a change of the queue head or the end of the simulation. |
| `pthread_mutex_t state_mutex` | `t_env` | Shared simulation state: `is_over`, `aborted`, `seq_counter`, each coder's `last_compile_start` and `compiles_count`, and a second copy of every dongle's `owner` (see below). |
| `pthread_mutex_t log_mutex` | `t_env` | Standard output, so one message is written at a time. |

No custom event system is used: the condition variables and the `is_over` flag are enough.

### How a coder takes its two dongles

```
take_pair():
    lock(state_mutex): deadline = last_compile_start + time_to_burnout
                       seq = ++seq_counter
    push request (id, deadline, seq) in the heap of dongle lo   # lock/unlock lo
    push request (id, deadline, seq) in the heap of dongle hi   # lock/unlock hi
    loop:
        lock(lo.mutex); lock(hi.mutex)                          # global order
        if simulation_over: unlock both, remove my requests, give up
        if lo and hi are free and cooled down
           and I do not have to give way at lo or at hi:
            lo.owner = hi.owner = me; remove my requests; broadcast both
            unlock both; break
        else:
            unlock the dongle I do not wait for
            wait on the other one (cond_wait if busy,
                                   timedwait until its cooldown or 1 ms otherwise)
            unlock it
    print "has taken a dongle" twice
```

"Give way at dongle D" means: the head of D's heap is another coder whose own other dongle is **not** held by anyone. Reading that dongle's owner requires `state_mutex`, which is why every write of `owner` (in `release_dongle` and in the take) is also done under `state_mutex`.

`release_dongle` locks the dongle, sets `owner = -1` (under `state_mutex` too), stores the new `cooldown_until` and calls `pthread_cond_broadcast` so every waiter re-evaluates the situation.

### Lock ordering

To rule out deadlocks between the mutexes themselves, locks are always nested in the same direction and never in reverse:

- `dongle.mutex` (lowest index first, then the highest) → `state_mutex` (when reading `is_over`, reading/writing an `owner`, or checking whether to give way),
- `log_mutex` → `state_mutex` (when printing),
- `state_mutex` is never held while locking a dongle mutex or `log_mutex`. `wake_all` is called after `state_mutex` is released.

The dongle mutexes are released before printing `has taken a dongle`, so a slow `printf` never blocks other coders from taking dongles.

### Race conditions prevented (examples)

- **Dongle duplication.** The test "are both dongles free?" and the assignment `owner = my_id` happen in the same critical section, with both dongle mutexes held, so two coders can never both believe they own a dongle, and a coder can never get only one of the two.
- **Lost wake-up at shutdown.** `is_over` is set under `state_mutex` *before* `wake_all` locks each dongle mutex. A coder re-checks `is_over` *after* locking the dongle mutexes and before waiting, so it is either already waiting on the condition variable (and gets the broadcast) or it sees `is_over = 1`. It can never go to sleep after the last broadcast.
- **Monitor vs coder data.** `last_compile_start` and `compiles_count` are written by coders and read by the monitor, always under `state_mutex`, so the monitor never decides on a torn or stale value.
- **Request ordering.** `seq_counter` is incremented under `state_mutex`, giving each request a unique, totally ordered sequence number that fifo and edf tie-breaks rely on. The same request is pushed in both dongle heaps, so both heaps agree on who has priority.
- **Counting and printing a compile.** The increment of `compiles_count`, the update of `last_compile_start`, the `printf` of `is compiling` and the check "was it the last one?" are a single critical section under `log_mutex` and `state_mutex`. The program therefore cannot stop before the last `is compiling` is printed, and nothing can be printed after it.
- **Double burnout / message after burnout.** The check of `is_over` and its update to `1` are done in a single critical section in `print_burnout`, so only the first burnout is printed and every later message is suppressed.

### Thread-safe communication between coders and the monitor

Coders and the monitor never exchange messages directly. They communicate only through the shared `t_env`, always under `state_mutex`:

- coders **publish** their progress (`last_compile_start`, `compiles_count`);
- the monitor **reads** it and, when needed, **raises** `is_over` and `aborted`;
- coders **read** `is_over` (`simulation_over`) in their loops and sleeps, and the broadcast from `wake_all` brings blocked ones back to check it.

## Testing

The program was compiled and run with ThreadSanitizer and AddressSanitizer (`-fsanitize=thread`, `-fsanitize=address,undefined`) on many parameter sets (1, 2, 3, 4, 5, 10, 50, 200 coders, `fifo` and `edf`, with and without cooldown, burnout and no-burnout cases) with no report. Useful checks:

```bash
./codexion 5 2000 200 200 200 7 0 edf | grep -c "is compiling"   # 35
./codexion 5 2000 200 200 200 7 0 edf | tail -1                  # last line is "is compiling"
valgrind --tool=helgrind ./codexion 5 800 200 200 200 3 0 edf
valgrind --leak-check=full ./codexion 5 800 200 200 200 3 0 fifo
```

## Resources

### References

- POSIX Threads manual pages: `pthread_create(3)`, `pthread_mutex_lock(3)`, `pthread_cond_wait(3)`, `pthread_cond_timedwait(3)`, `pthread_cond_broadcast(3)`, `gettimeofday(2)`, `usleep(3)`.
- E. G. Coffman, M. J. Elphick, A. Shoshani, *System Deadlocks*, ACM Computing Surveys, 1971 (the four deadlock conditions).
- E. W. Dijkstra, *Hierarchical ordering of sequential processes*, 1971 (dining philosophers and resource ordering).
- C. L. Liu, J. W. Layland, *Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment*, 1973 (Earliest Deadline First).
- T. H. Cormen et al., *Introduction to Algorithms*, chapter on heaps and priority queues.
- A. Downey, *The Little Book of Semaphores* (classic synchronization problems).

### Use of AI

An AI assistant (Claude) was used during development, for these tasks:

- explaining concepts (threads, mutexes, condition variables, heaps, Valgrind, real time vs CPU time) and reviewing the notes written for this project;
- reviewing the code against the subject and looking for concurrency problems (lost wake-ups, lock ordering, shutdown handling, memory use of the heaps);
- diagnosing a starvation problem found by testing (the "convoy" effect of taking one dongle and then waiting for the other) and proposing the fix described above (atomic acquisition of both dongles, the rule of giving way only to a coder that can run);
- proposing and adjusting the logic that counts compiles and ends the simulation on the last `is compiling`;
- generating test commands (burnout and no-burnout cases) and running the sanitizers;
- helping to write this README.

Every proposal was tested with sanitizers and with the examples above, and the AI output was reviewed before being kept.