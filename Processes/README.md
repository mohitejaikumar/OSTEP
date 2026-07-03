# Processes — `process-run.py` Homework Analysis

Analysis of the scheduling simulator (`process-run.py`) for the OSTEP
"Processes" chapter homework.

## How the model works

Each process is a string of instructions that are either **CPU** or **IO**.
The key rule about I/O in this simulator:

- Issuing an I/O takes **1 tick** (`RUN:io`), then the process sits
  **BLOCKED** for the I/O length (default **5 ticks**, counted under `IOs`
  busy), then it needs **1 more tick** (`RUN:io_done`, marked with `*`) to
  wrap up before its next instruction.

A process list like `-l 3:50` means 3 instructions, each with a **50% chance**
of being a CPU op vs an I/O op. That randomness (seeded with `-s`) is why the
same process list behaves differently per seed.

## The three seeds (`-s 1/2/3 -l 3:50,3:50`)

| Seed | What the RNG rolled                              | Total time | CPU util |
|------|--------------------------------------------------|-----------:|---------:|
| 1    | p0 = cpu, io, io ; p1 = 3×cpu                     | 15         | 53%      |
| 2    | both processes I/O-heavy (5 I/Os between them)    | 16         | 62%      |
| 3    | p0 = cpu, io ; p1 = io, io, cpu                   | 18         | 50%      |

What to predict *before* running: whenever a process hits an `io` instruction
it goes BLOCKED, and under the default `SWITCH_ON_IO` the CPU switches to the
other process.

- **Seed 2** shows I/O parallelism — both processes are BLOCKED on I/O at the
  same time (`IOs = 2`), i.e. two devices busy at once.
- **Seed 3** shows the opposite failure — at times 12–16 p1 is BLOCKED and p0
  is already DONE, so the CPU sits **idle** with nobody to run.

## `-I IO_RUN_IMMEDIATE` vs `-I IO_RUN_LATER`

Controls **what happens the instant an I/O completes** while another process
is currently running.

- **`IO_RUN_LATER`** (default): the process whose I/O just finished goes to the
  **back** of the ready queue; the currently-running process keeps going.
- **`IO_RUN_IMMEDIATE`**: the process whose I/O just finished **runs right
  away**, jumping the queue.

Made visible with an I/O-bound + a CPU-bound process (`-l 4:0,4:100 -L 2`):

```
IO_RUN_LATER      -> Total Time 18   (CPU 67%)
IO_RUN_IMMEDIATE  -> Total Time 16   (CPU 75%)
```

**Why IMMEDIATE wins:** you want the I/O-heavy process kept "in flight" — the
moment its I/O finishes it should issue its next I/O so the device stays busy,
while the CPU-bound job fills the CPU in between. With `LATER` the I/O process
waits behind the CPU hog, the device goes idle, and everything stretches out.
This is the classic **keep-the-device-busy / overlap** argument. IMMEDIATE is
generally the better default; the caveat is it preempts a running job more
aggressively.

> Note: for **seed 2** specifically, `-I` makes **no difference** (16 either
> way) because whenever an I/O completed there, the other process was already
> BLOCKED — there was no queue decision to make. The flag only matters when a
> *ready* process is competing for the CPU.

## `-S SWITCH_ON_IO` vs `-S SWITCH_ON_END`

Controls **when the scheduler is allowed to switch processes**.

- **`SWITCH_ON_IO`** (default): when a process issues I/O and blocks, switch to
  another process — overlap, CPU stays useful.
- **`SWITCH_ON_END`**: **never switch until the running process is completely
  DONE.** A blocked process holds the CPU hostage.

Seed 2 makes the cost brutal:

```
SWITCH_ON_IO   -> Total Time 16   (CPU 62%)
SWITCH_ON_END  -> Total Time 30   (CPU 33%)   <- p1 sits READY, idle, the whole time
```

In the `SWITCH_ON_END` trace, p1 is stuck in **READY** from time 1 to 15 — it
is runnable the entire time but the scheduler refuses to touch it because p0
has not finished. Meanwhile p0 spends most of those ticks BLOCKED on I/O with
the CPU doing nothing. Total time nearly **doubles** and CPU utilization
**halves**.

## Key takeaways

1. **`SWITCH_ON_IO` >> `SWITCH_ON_END`** when processes do I/O — overlapping
   one process's I/O with another's CPU is the whole point of multiprogramming.
2. **`IO_RUN_IMMEDIATE` generally beats `IO_RUN_LATER`** for throughput because
   it keeps I/O devices continuously busy — but only matters when there is a
   ready process to schedule against.
3. Utilization is highest when you can **overlap CPU work with I/O wait**; it
   collapses whenever the CPU is forced to sit idle (either by `SWITCH_ON_END`,
   or by a workload where the only runnable process is blocked).

## Reproducing

```sh
# The three seeds with default behavior
./process-run.py -s 1 -l 3:50,3:50 -c -p
./process-run.py -s 2 -l 3:50,3:50 -c -p
./process-run.py -s 3 -l 3:50,3:50 -c -p

# IO completion behavior (contrast is clearest with IO-heavy + CPU-heavy)
./process-run.py -l 4:0,4:100 -L 2 -I IO_RUN_LATER     -c -p
./process-run.py -l 4:0,4:100 -L 2 -I IO_RUN_IMMEDIATE -c -p

# Switch behavior
./process-run.py -s 2 -l 3:50,3:50 -S SWITCH_ON_IO  -c -p
./process-run.py -s 2 -l 3:50,3:50 -S SWITCH_ON_END -c -p
```

Flags: `-c` computes/prints the trace, `-p` prints stats, `-L` sets I/O length.
