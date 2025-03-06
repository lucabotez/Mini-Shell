Copyright @lucabotez

# Mini Shell

## Overview
**Mini Shell** is a minimal, custom-built shell for Unix-like systems, designed to execute **built-in commands**, manage **external processes**, and support **input/output redirection and pipelines**.

## Features
- **Built-in commands**: `cd`, `pwd`, `exit`
- **Environment variable support**
- **Execution of external commands**
- **Input and output redirection** (`>`, `>>`, `<`)
- **Piping support** (`|`)
- **Parallel execution of commands** (`&`)
- **Conditional execution** (`&&`, `||`)

## Implementation Details
### **1. Built-in Commands**
- `cd <directory>` – Changes the current working directory.
- `pwd` – Prints the current working directory.
- `exit` – Exits the shell.

### **2. External Command Execution**
- Commands not recognized as built-in are executed using `execvp()`.
- Supports argument passing and variable expansion.

### **3. Input & Output Redirection**
- `< file` – Reads input from a file.
- `> file` – Redirects output to a file (overwrite).
- `>> file` – Appends output to a file.

### **4. Piping (`|`)**
- Allows output from one command to be used as input for another.
- Implemented using anonymous pipes (`pipe()` and `dup2()`).

### **5. Parallel Execution (`&`)**
- Runs multiple commands simultaneously using `fork()`.
- Waits for child processes to finish using `waitpid()`.

### **6. Conditional Execution**
- `&&` – Executes the second command only if the first succeeds.
- `||` – Executes the second command only if the first fails.

## Notes
- The shell uses **low-level system calls** to handle processes.
- Redirection and piping are implemented using `dup2()`.
