# Section 1.2.2 Video Walkthrough Script

Use this as a teleprompter while recording. Each step tells you what to say and exactly where to scroll in the code. Aim for 2:30-3:00 total.

---

## Step 1: Compile-Time Memory Sizing (~30s)

**Open:** `shellmemory.h`, scroll to lines 3-24

**Say:**

> So for 1.2.2, we're extending the shell with demand paging -- programs no longer need to fully fit in memory to run.
>
> The first thing we did was make the memory sizes configurable at compile time. You can run `make mysh framesize=X varmemsize=Y`, and those values get passed as `-D` flags to gcc, which replace the macros `FRAME_STORE_SIZE` and `VAR_STORE_SIZE` with the actual numbers.
>
> Each frame holds exactly 3 lines of code, and the total number of frames is just the frame store size divided by 3.

**Then open:** `shell.c`, scroll to line 14

**Say:**

> At startup, the shell prints the frame store size and variable store size instead of the old version message -- you can see that right here.

---

## Step 2: The Backing Store (~30s)

**Open:** `interpreter.c`, scroll to lines 440-453 (the `backing_entry` struct)

**Say:**

> The key data structure we introduced is the backing store. When a program is loaded, we read every single line of the script into this `backing_entry` struct -- it's essentially an in-memory copy of the file. It stores the filename, an array of all the lines, the line count, and the page count.
>
> The reason we keep this around is so that when a page fault happens later, we can pull the missing lines straight from memory without ever re-opening the file.

---

## Step 3: Lazy Loading (~40s)

**Stay in:** `interpreter.c`, scroll to lines 686-750 (`load_file_into_frames`)

**Say:**

> Here's `load_file_into_frames`. First, we open the script file and read every line into the backing store.

*Point to lines 709-714 (the fgets loop)*

> Then we build the pagetable. Every entry starts as negative one, meaning "not yet loaded."

*Point to line 724*

> But we don't load everything into the frame store right away. We only push the first two pages -- so the first 6 lines -- into actual frames.

*Point to lines 729-730*

> Everything else stays as negative one and will be loaded on demand when the process actually needs those instructions. That's the "demand" in demand paging.

---

## Step 4: Page Fault Detection and Handling (~40s)

**Scroll to:** lines 461-464 (`next_page_is_loaded`)

**Say:**

> When a process is about to execute an instruction, we check whether the page it needs is in memory. If the pagetable entry is negative one, that's a page fault.

**Scroll to:** lines 467-477 (`handle_page_fault` top)

**Say:**

> In the page fault handler, first we look up the missing page's lines in the backing store -- so we never need to re-open the file.

*Point to lines 489-493 (the `allocate_frame` call and the `if (frame >= 0)` branch)*

> If there's a free frame available, we just load the page in and update the pagetable. Simple.

*Point to lines 496-518 (the `else` branch with eviction)*

> If the frame store is full, we pick the least recently used frame as our victim, print its contents, evict it, and load the new page into that slot.

---

## Step 5: The Reverse Map and Invalidation (~30s)

**Scroll to:** lines 425-434 (`frame_owners` struct and array)

**Say:**

> The tricky part is invalidation. We maintain a reverse map called `frame_owners`. For each frame, it tracks which program and which logical page currently occupies it.

**Scroll to:** lines 520-532 (the invalidation loop inside `handle_page_fault`)

**Say:**

> When we evict a frame, we use this reverse map to walk through every PCB in the ready queue and set their pagetable entry to negative one if they were referencing that frame. This is especially important when two processes are running the same script -- they share frames, so both need to be invalidated.
>
> After the fault is handled, the process resumes normally on its next turn in the queue, and by then the page is loaded and ready to execute.

---

## Timing Checklist

| Step | Topic                          | Target |
|------|--------------------------------|--------|
| 1    | Compile-time sizing            | ~30s   |
| 2    | Backing store struct           | ~30s   |
| 3    | Lazy loading                   | ~40s   |
| 4    | Page fault detection + handler | ~40s   |
| 5    | Reverse map + invalidation     | ~30s   |
|      | **Total**                      | **~2:50** |

## Quick Tips

- Have all files already open in tabs before recording so you don't waste time searching.
- Tab order: `shellmemory.h` -> `shell.c` -> `interpreter.c` (you stay in interpreter.c for steps 2-5).
- Scroll slowly so the viewer can follow along.
- If you go over 3:00, trim the backing store explanation (step 2) since it's the simplest part.
