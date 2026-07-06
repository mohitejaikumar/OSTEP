# Direct Execution — Homework Notes

Measuring the cost of a system call and a context switch, from the OSTEP
"Mechanism: Limited Direct Execution" chapter homework.

Measured on: Apple M4 Pro, macOS 26.3.1 (arm64).

## `timer_precision.c` — how precise is `gettimeofday()`?

Two experiments:

1. Call `gettimeofday()` back-to-back a million times and look at the
   distribution of deltas.
2. Spin-poll until the reported value actually changes from a fixed
   starting reading, and count how many calls that took.

```
back-to-back gettimeofday(): 981613/1000000 calls returned the same value as the previous call
smallest observed nonzero delta between back-to-back calls: 1 us
clock advanced by 1 us after 20 calls (tick granularity)
```

- `gettimeofday()` **reports** microsecond values, but its actual
  resolution is coarser: ~98% of back-to-back calls return the exact same
  reading, and it takes on the order of 20 calls before the clock visibly
  advances by even 1 us.
- This means a single call to `gettimeofday()` is way faster than 1 us
  (it's a vDSO/commpage read on this platform, not a real trap into the
  kernel) — so **timing one syscall directly would be dominated by clock
  granularity, not by the syscall itself.** This is exactly the problem
  the assignment warns about, and it's why `syscall_cost.c` and
  `context_switch.c` both time a large batch of iterations and divide,
  rather than timing a single call.
- On x86 Linux, `rdtsc` gives a much finer-grained cycle counter for this;
  it isn't available on Apple Silicon, so the batching approach is used
  here instead (`mach_absolute_time()` would be another option, but
  `gettimeofday()` is precise enough once batched over ~10^6 iterations).

## `syscall_cost.c` — cost of a system call

Repeatedly calls `read(fd, buf, 0)` on a valid fd (`/dev/zero`, opened
once outside the timed loop) — a 0-byte read still has to enter the
kernel, resolve the fd, and return, but does no actual data copying.

```
$ ./syscall_cost 2000000
2000000 calls to read(fd, buf, 0) took 461406.00 us total
average system-call cost: 0.2307 us   (three runs: 0.2307, 0.2334, 0.2264 us)
```

- ~0.23 us (230 ns) per system call, averaged over 2,000,000 iterations —
  comfortably above the ~1 us clock granularity found above, since the
  *total* loop time (hundreds of milliseconds) is what's being measured,
  not any single call.
- Using a real (if trivial) fd rather than an invalid one avoids skewing
  the result with whatever fast-path error handling an invalid-fd read
  might take.

## `context_switch.c` — cost of a context switch

lmbench-style measurement: parent and child ping-pong a single byte over
two pipes (parent → child, child → parent). Each round trip forces the
child to block in `read()` while the parent runs, then the parent to
block while the child runs — i.e. **two context switches per round
trip**, so the average is `total_time / (iterations * 2)`.

```
$ ./context_switch 200000
200000 ping-pong round trips took 957338.00 us total
average context-switch cost: 2.3933 us   (three runs: 2.39, 2.33, 2.29, 2.28 us)
```

- ~2.3 us per context switch — roughly **10x the cost of a plain system
  call**, consistent with the intuition that a context switch does
  everything a syscall does (trap into the kernel) plus saving/restoring
  a full register set and switching the scheduler's notion of "current
  process."
- **CPU affinity**: on Linux, `sched_setaffinity()` pins both processes to
  CPU 0 so the measurement reflects same-core switch cost rather than
  cross-core migration. macOS has no equivalent — `thread_policy_set()`
  affinity tags are only an advisory hint to the scheduler, not a hard
  pin — so `pin_to_cpu0()` is a no-op on macOS (guarded by `#ifdef
  __linux__`). This measurement is therefore best-effort on this machine;
  the Linux path is there for when this is run on a Linux box/VM.

## Reproducing

```sh
gcc -O2 -Wall -o timer_precision timer_precision.c
gcc -O2 -Wall -o syscall_cost syscall_cost.c
gcc -O2 -Wall -o context_switch context_switch.c

./timer_precision
./syscall_cost 2000000
./context_switch 200000
```
