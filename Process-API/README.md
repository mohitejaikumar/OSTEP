# Process API — Homework Notes

Notes and observed behavior for each program in this directory, from the
OSTEP "Process API" chapter homework.

## `p1.c` — fork() and variable copies

Sets `x = 100` before `fork()`, then has each process modify its own `x`.

- The child gets its **own copy** of `x` (same value, `100`, as the parent
  had at the moment of `fork()`) — not a reference to the parent's memory.
  `fork()` duplicates the entire address space (copy-on-write under the
  hood), so afterward the two processes have fully independent memory.
- Parent does `x + 20 = 120`, child does `x - 20 = 80` — neither sees the
  other's change, confirming the copies are independent.
- **Buffering gotcha**: when stdout isn't a real terminal (e.g. piped),
  `printf`'s `stdio` buffer is fully buffered instead of line-buffered. The
  `"before fork"` line was still sitting in the buffer at `fork()` time, got
  copied into the child's memory along with everything else, and both
  processes flushed their own copy independently on exit — so it printed
  **twice**. Run directly in a terminal and it prints once.

## `p2.c` — fork() + shared vs. independent file descriptors

Explores what happens when parent and child write to the same file
concurrently, depending on *when* and *how* `open()` is called.

Mental model — three-level table:

```
per-process fd table  →  open file description (offset, flags)  →  inode (file on disk)
     (private)                    (can be shared or not)
```

- **`open()` before `fork()` (shared fd):** the child's fd table entry is
  copied, but both entries point to the **same open file description** —
  i.e. the **same offset**, one object in the kernel. Every `write()` from
  either process atomically advances that one shared offset, so writes
  always land back-to-back. Result: no data loss, just unpredictable
  ordering. Confirmed: 2000/2000 lines all present.
- **`open()` independently in each process (separate fd), no `O_TRUNC`:**
  each process gets its **own offset**, both starting at 0. Writes from
  each process land at the same byte positions and overwrite each other.
  Result: only the last writer's content survives (in our run, 1000 lines,
  all `child`, because line lengths happened to match exactly, hiding any
  fragment garbage).
- **Adding `O_TRUNC`:** truncates the file at `open()` time — doesn't fix
  the offset race, just means whichever `open()` runs last wipes what the
  other already wrote.
- **Adding `O_APPEND` (no shared fd):** fixes it! Each `write()` becomes an
  atomic "seek-to-current-EOF, then write" as one kernel operation, so even
  with independent offsets, writes never collide — this is what let separate
  `open()` calls behave safely.
- **Interleaving pattern (why not "all parent then all child"):** parent and
  child are two independently-scheduled processes. The OS time-slices
  (preemptive scheduling) or runs them truly in parallel on separate cores,
  so output comes in unpredictable bursts. `fork()` gives **no ordering
  guarantee** between parent/child execution — this varies from run to run.

## `p3.c` — ensuring child prints first, without `wait()`

Child prints `"hello"`, parent prints `"goodbye"`, guaranteed ordering, no
`wait()` call allowed.

- Regular variables are **not shared** across `fork()` — each process gets
  an independent copy, so a plain flag variable can't be used to signal
  between them.
- Solution: **`mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0)`**
  before `fork()` — allocates a page of memory with no backing file
  (`MAP_ANONYMOUS`) but marked shared (`MAP_SHARED`), so the child inherits
  the *same* physical mapping instead of a private copy.
- Child sets `*done = 1` after printing; parent busy-spins
  (`while (*done == 0);`) until it sees the flag, then prints. Works
  reliably every run — but burns CPU spinning, which is exactly the
  motivating pain point for why `wait()` (blocking, no busy-poll) exists.
- **`mmap()` flags recap:**
  - `PROT_READ` / `PROT_WRITE` / `PROT_EXEC` / `PROT_NONE` — CPU-enforced
    page permissions (mismatched access → `SIGSEGV`).
  - `MAP_SHARED` vs `MAP_PRIVATE` — shared writes are visible across
    processes mapping the same region (incl. after `fork()`); private
    writes are copy-on-write and invisible to others.
  - `MAP_ANONYMOUS` — no file backing, just zeroed memory from the kernel.

## `p4.c` — all the `exec()` variants running `/bin/ls`

Forks once per variant (`execl`, `execle`, `execlp`, `execv`, `execvp`,
`execvpe` on Linux only — it's a GNU/glibc extension, unavailable on macOS).

- Naming decodes into independent axes:
  - **`l` vs `v`** — args as a variadic list vs. an array (`argv[]`).
  - **`p`** — search `$PATH` to resolve a bare command name.
  - **`e`** — caller supplies an explicit `envp[]` instead of inheriting the
    current environment.
- All variants are thin userspace wrappers around the one real syscall,
  **`execve(path, argv, envp)`** — the differences are just convenience for
  different call sites (fixed known args vs. dynamically built argv, PATH
  resolution or not, custom environment or not).
- **Buffering gotcha again**: without `fflush(stdout)` after each label
  `printf`, all five `ls -l` outputs (unbuffered, from a separate exec'd
  program) appeared before any of the parent's buffered label lines. Fixed
  with explicit `fflush(stdout)` calls around each variant.

## `p5.c` — `wait()` in parent vs. in child

- In the **parent**, `wait(&status)` blocks until a child exits and returns
  that **child's pid**; `status` is decoded with `WIFEXITED(status)` /
  `WEXITSTATUS(status)`.
- In the **child**, `wait()` fails immediately: returns **`-1`**, with
  `errno == ECHILD` ("No child processes") — `wait()` only reaps a
  process's own direct children, and this child has none of its own.
- Timing detail: the child's failed `wait()` returns instantly, while the
  parent's real `wait()` blocks — so the child's message can print before
  the parent's, even though the parent called `wait()` first in program
  order.

## `p6.c` — `waitpid()` for a specific child

Two children: **A** sleeps 2s before exiting, **B** exits immediately.
Parent calls `waitpid(pidB, ...)` first, deliberately targeting the child
that will finish quicker, and gets the right exit status regardless of
actual fork order.

- `waitpid(pid, &status, options)` lets you wait for a **specific** child,
  unlike `wait()` which returns whichever child happens to exit first.
- `waitpid(-1, &status, 0)` behaves the same as plain `wait()`.

## `p7.c` — non-blocking wait with `WNOHANG`

Same idea as `p6.c`, but polls: `waitpid(rc, &status, WNOHANG)` returns `0`
immediately if the child hasn't exited yet (instead of blocking), letting
the parent do other work in a loop until the child finally finishes.

- Useful for: shells' job control (checking background jobs without
  freezing the prompt), process supervisors polling multiple workers each
  tick, event loops that can't afford to block on a child.

## `p8.c` — closing `STDOUT_FILENO` before `printf()`

Child does `close(STDOUT_FILENO)` then `printf("hello from child\n")`.

- `printf()` itself still "succeeds" (returns 17, the formatted length) —
  it just copies into `stdio`'s userspace buffer and has no idea yet that
  fd 1 is closed.
- The real failure only shows up at **`fflush(stdout)`**, which is when
  `stdio` actually issues `write(1, ...)` — returns `-1`, `errno == EBADF`
  ("Bad file descriptor"). The buffered data is discarded, unrecoverable.
- Bonus gotcha (not exercised in code, just noted): `open()` always assigns
  the **lowest available fd**. After closing fd 1, a subsequent `open()` in
  the same process would likely be handed fd 1 back — silently redirecting
  any later `printf`/`fflush` into that unrelated file instead of failing.
  This is the exact mechanism shells use to implement `>` redirection
  before `exec`-ing a program.

## `p9.c` — connecting two children with `pipe()`

Child A's stdout → pipe → Child B's stdin, running `ls -l | wc -l`
end-to-end via `pipe()` + `dup2()` + `exec()`.

- `pipe(fds)` creates one pipe with `fds[0]` (read end) and `fds[1]` (write
  end).
- Each child uses `dup2()` to alias its stdin/stdout onto the relevant pipe
  end *before* `exec()`, so the exec'd program (`ls`, `wc`) is wired to the
  pipe transparently, with no cooperation needed from the program itself.
- **fd table vs. underlying object**: `fork()` gives each process its own
  **copy of the fd table** (so closing an fd number in one process doesn't
  affect another's copy of that number) — but every copy of a given pipe fd,
  across parent and both children, references the **same underlying pipe
  object** with one shared reference count.
- **Why the parent must close both pipe ends after forking both children**:
  the pipe's write end is only truly "closed" (triggering EOF for the
  reader) once *every* process's copy of that fd is closed. If the parent
  keeps holding its copy of the write end open, child B's `read()` blocks
  forever waiting for EOF that never comes — a common source of hung
  pipelines.
