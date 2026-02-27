# McGill Shell - Custom Shell with CPU Scheduling Algorithms

A feature-rich shell implementation in C with integrated CPU process scheduling algorithms, designed as part of McGill University's ECSE 427 (Operating Systems) course.

## Overview

This project implements a custom Unix-like shell with an embedded scheduler supporting multiple CPU scheduling policies. The shell can execute individual scripts synchronously or schedule multiple scripts concurrently using various algorithms including FCFS, SJF, Round-Robin, and Aging.

## Features

### Shell Commands

- **help** - Display all available commands
- **quit / exit** - Terminate the shell
- **set VAR STRING** - Assign a value to shell memory
- **print VAR** - Display the value of a variable
- **echo STRING** - Print a string to stdout
- **my_ls** - List files in current directory (custom implementation with case-insensitive sorting)
- **my_mkdir NAME** - Create a new directory (name must be alphanumeric or variable reference)
- **my_touch PATH** - Create a new file
- **my_cd PATH** - Change working directory
- **source SCRIPT.TXT** - Execute shell commands from a file synchronously
- **exec PROG1 [PROG2 PROG3] POLICY [OPTIONS]** - Schedule 1-3 programs with a specific scheduling policy
- **run COMMAND [ARGS...]** - Execute external system commands via fork/exec

### Scheduling Policies

The **exec** command supports the following CPU scheduling algorithms:

#### Non-Preemptive Policies

- **FCFS (First Come First Served)** - Programs run to completion in the order they are submitted
- **SJF (Shortest Job First)** - Programs are sorted by length (number of instructions) before execution; shortest programs run first

#### Preemptive Policies

- **RR (Round-Robin, quantum=2)** - Programs execute for a maximum of 2 instructions before being preempted and moved to the back of the queue
- **RR30 (Round-Robin, quantum=30)** - Programs execute for a maximum of 30 instructions before preemption
- **AGING** - Programs are sorted by a "job_length_score" that decreases by 1 each time slice. Shorter jobs get higher priority as they age

### Advanced Features

- **Batch Mode** - Load commands from stdin (e.g., `./mysh < input.txt`)
- **Command Chaining** - Use semicolons to chain multiple commands: `source prog1; exec prog2 prog3 FCFS;`
- **Background Execution** - Append `#` flag to exec to run programs asynchronously while allowing the shell to accept more input
- **Multi-threaded Mode** - Append `MT` flag to exec for thread-based worker pool execution (2 worker threads)
- **Program Variables** - Use `$VARIABLE` syntax to reference stored values in my_mkdir and other commands

## Architecture

### Core Components

#### **Shell Memory** (`shellmemory.c/h`)
- Variable storage (1000 slots max)
- Program line storage for loaded scripts
- Functions for loading scripts from files or stdin

#### **Scheduler** (`scheduler.c/h`)
- Process Control Block (PCB) data structure for tracking process execution state
- Ready queue implementation with linked-list backend
- Policy-specific enqueue logic (FCFS, SJF, AGING)
- Thread-safe variants for multi-threaded execution

#### **Interpreter** (`interpreter.c/h`)
- Command parser and dispatcher
- Implementation of all shell built-in commands
- Execution engine for running ready queue until completion
- Worker thread management for MT mode

#### **Shell** (`shell.c/h`)
- Main shell loop (REPL)
- Input parser with support for command chaining
- Interactive and batch mode handling

### Data Structures

#### Process Control Block (PCB)
```c
struct PCB {
    int pid;                    // Unique process ID
    int start_index;            // Starting index in shell memory
    int length;                 // Total number of instructions
    int pc;                     // Program counter (current instruction)
    int job_length_score;       // Sorting key for AGING policy
    struct PCB *next;           // Pointer to next PCB in queue
};
```

#### Ready Queue
```c
struct ReadyQueue {
    struct PCB *head;           // First PCB (next to execute)
    struct PCB *tail;           // Last PCB (most recently added)
};
```

## Building the Project

### Requirements
- GCC compiler
- POSIX-compliant Unix/Linux environment
- pthread library for multi-threaded support

### Compilation

```bash
cd src
make
```

This generates the `mysh` executable.

#### Clean Build
```bash
make clean     # Remove compiled objects and executable
make style     # Format code with indent
```

## Usage

### Interactive Mode
```bash
./mysh
$ set message "Hello World"
$ print message
$ my_mkdir testdir
$ my_cd testdir
$ source script.txt
$ quit
```

### Batch Mode
```bash
./mysh < input_commands.txt
```

### Create Test Programs
Test programs are simple text files containing shell commands (one per line):

**P_prog1.txt:**
```
echo P1L1
echo P1L2
echo P1L3
```

**P_prog2.txt:**
```
echo P2L1
echo P2L2
```

### Execute with Scheduling

#### FCFS Scheduling
```bash
exec P_prog1 P_prog2 FCFS
```
Output: P1L1, P1L2, P1L3, P2L1, P2L2 (P_prog1 completes first, then P_prog2)

#### Round-Robin Scheduling
```bash
exec P_prog1 P_prog2 RR
```
Output: P1L1, P2L1, P1L2, P2L2, P1L3 (alternates every 2 instructions)

#### Shortest Job First
```bash
exec P_prog1 P_prog2 SJF
```
Output: P2L1, P2L2, P1L1, P1L2, P1L3 (P_prog2 is shorter, runs first)

#### Background/Asynchronous Execution
```bash
exec P_prog1 P_prog2 FCFS #
```
The shell returns to the prompt immediately and executes the programs in the background.

#### Multi-threaded Mode
```bash
exec P_prog1 P_prog2 P_prog3 RR MT
```
Uses 2 worker threads for concurrent execution with round-robin scheduling.

## Test Suite

Comprehensive test cases are provided in `src/test-cases/`:

### Test Structure
- `T_*.txt` - Input commands to test
- `T_*_result.txt` - Expected output

### Running Tests

```bash
cd src/test-cases
../mysh < T_exec_single.txt        # Manual test
diff <(../mysh < T_exec_single.txt) T_exec_single_result.txt  # Compare output

./run_exec_tests.sh                # Run all tests automatically
```

### Test Categories

- **Exec Tests** - Basic exec functionality (single/dual/triple programs)
- **Policy Tests** - FCFS, SJF, RR, RR30, AGING scheduling
- **Error Tests** - Invalid policies, missing files, duplicate names
- **Background Tests** - Asynchronous execution with `#` flag
- **MT Tests** - Multi-threaded execution tests
- **Aging Tests** - Verification of AGING algorithm behavior

## Algorithm Details

### FCFS (First Come First Served)
- Simplest, non-preemptive policy
- Programs execute in submission order to completion
- Implementation: append to tail of queue, dequeue from head

### SJF (Shortest Job First)
- Non-preemptive, based on program length
- Shorter programs have higher priority
- Implementation: sort by `length` field on enqueue

### Round-Robin (RR)
- Preemptive with time quantum
- Each process runs for up to `quantum` instructions before returning to back of queue
- If process completes before quantum expires, it is freed
- Supports configurable quantum (RR=2, RR30=30)

### AGING
- Preemptive with aging mechanism
- Each PCB has `job_length_score` initialized to program length
- After each time slice, age all remaining processes: `job_length_score--` (floor at 0)
- Processes are sorted by score (lower = higher priority)
- Prevents indefinite starvation of long-running jobs

## Implementation Notes

### Memory Management
- Shell memory: fixed 1000-slot array for variables and program lines
- Dynamic allocation: PCBs and command strings are malloc'd
- Memory is freed when: programs complete, shell exits, or memory is explicitly cleared

### Synchronization (Multi-threaded Mode)
- Mutex guards shared ready queue (`rq_mutex`)
- Condition variables coordinate worker threads with main thread
- `rq_not_empty` - signals workers that work is available
- `rq_all_done` - signals main thread when all work is complete and no workers are active

### Error Handling
- Invalid commands return error code 1 (Unknown Command)
- File not found returns error code 3
- Invalid policies are rejected with descriptive messages
- Duplicate program names in exec are detected and rejected

## Project Structure

```
Part 2/
├── README.md
├── A1solution/          # Reference implementation (Part 1)
│   ├── shell.c
│   ├── interpreter.c
│   └── ...
└── src/
    ├── shell.c
    ├── shell.h
    ├── interpreter.c
    ├── interpreter.h
    ├── scheduler.c
    ├── scheduler.h
    ├── shellmemory.c
    ├── shellmemory.h
    ├── Makefile
    └── test-cases/          # Comprehensive test suite
        ├── P_*.txt          # Test program files
        ├── T_*.txt          # Test input files
        ├── T_*_result.txt   # Expected output files
        └── run_exec_tests.sh
```

## Debugging

Enable debug output by uncommenting `#define DEBUG 1` in `interpreter.c`:

```c
#define DEBUG 1
```

When enabled, prints parser tokens and debug messages to stderr.

## Author & Course

- **Course**: McGill University ECSE 427 (Operating Systems)
- **Semester**: Winter 2026
- **Assignment**: Part 2 - CPU Scheduling

## License

Academic project - Use for educational purposes only.

## Known Limitations

- Maximum of 3 programs per exec command
- Maximum 1000 lines of program code
- Background execution (`#` flag) prevents access to shell commands until all programs complete
- MT mode creates exactly 2 worker threads
- No support for pipes, redirection, or advanced shell features

## Future Enhancements

- Support for more than 3 concurrent programs
- Additional scheduling algorithms (Priority Queues, Multilevel Feedback)
- Signal handling for program termination
- Expanded shell command set
- File I/O redirection and piping
