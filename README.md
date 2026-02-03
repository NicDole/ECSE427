# ECSE 427 – Assignment 1  
## Simple Shell Implementation

### Authors
- **S. Nicolas Dolgopolyy** – 261115875  
- **Timothe Roma** – 261081874  

---

## Description
This project implements a simple Unix-like shell in C, based on the provided starter code.  
The shell supports both interactive and batch execution modes and implements all required built-in commands as specified in the assignment description.

The shell parses user input, executes built-in commands directly, and supports running external programs using `fork` and `execvp`.

---

## Implemented Features

### Core Functionality
- Interactive mode with prompt
- Batch mode via input redirection
- Graceful termination using `quit`
- Proper handling of end-of-file in batch mode
- Support for one-line command chaining using `;`

### Built-in Commands
- `help` – Displays all supported commands  
- `quit` – Exits the shell with a goodbye message  
- `set VAR STRING` – Stores a variable in shell memory  
- `print VAR` – Prints the value of a stored variable  
- `echo TOKEN` – Prints a token or the value of a variable  
- `source SCRIPT.TXT` – Executes commands from a script file  
- `my_touch FILE` – Creates an empty file (alphanumeric names only)  
- `my_cd DIR` – Changes the current directory (alphanumeric names only)  
- `my_ls` – Lists files and directories in the current directory in alphabetical order  
- `my_mkdir DIR` – Creates a directory, supports `$VAR` expansion with validation  
- `run COMMAND ARGS` – Executes external commands using `fork` and `execvp`  

---

## Notes on Behavior
- Files and directories created during execution are **not automatically removed**. This matches expected shell behavior and the assignment specification.
- Variable expansion for `my_mkdir` follows the assignment rules strictly. Invalid or undefined variables result in `Bad command: my_mkdir`.
- Output ordering is consistent in both interactive and batch modes.
- The shell uses the provided starter code structure and extends it to meet all assignment requirements.

---

## Compilation and Execution

Compile using:
```bash
make
```

Run interactively
```bash
./mysh
```

Run in batch mode 
```bash
./mysh < script.txt
```
---

## Starter Code
This project is based on the starter code provided with the assignment. All modifications and extensions were implemented on top of that base.

## Assumptions
No additional assumptions were made beyond what is explicitly stated in the assignment description.