### Project Title: Unix Shell -- wsh

#### Short Description:
This project involves building a simple Unix shell named wsh in C. The shell acts as a command-line interpreter, creating child processes to execute commands entered by the user. It supports interactive and batch modes, basic shell functionalities, environment variables, shell variables, piping, history tracking, and built-in commands like exit, cd, export, local, vars, and history.

#### Readme:

##### Unix Shell -- wsh

**Usage:**
- To run the shell interactively:
  ```
  prompt> ./wsh
  wsh>
  ```
- To run the shell with a batch file:
  ```
  prompt> ./wsh script.wsh
  ```

**Features:**
- **Basic Shell (wsh):**
  - Interactive loop with prompt `wsh>`.
  - Supports parsing and executing commands.
- **Pipes:**
  - Supports pipe redirection (`|`) for composing multiple commands.
- **Environment and Shell Variables:**
  - Handles environment and shell variables.
  - Supports `export`, `local`, and `vars` built-in commands.
- **Paths:**
  - Utilizes `PATH` environment variable to locate executables.
- **History:**
  - Tracks the last five commands executed by the user.
  - Supports configurable history length and executing commands from history.
- **Built-in Commands:**
  - `exit`, `cd`, `export`, `local`, `vars`, `history`.

**Implementation:**
- **Structure:**
  - Uses a while loop for the main shell operation.
  - Utilizes `getline()` for reading input lines.
  - Supports both interactive and batch modes.
- **Parsing:**
  - Uses `strsep()` to parse input lines into constituent pieces.
- **Execution:**
  - Uses `fork()`, `exec()`, and `wait()`/`waitpid()` for process management.
  - Implements `execvp()` for command execution.
- **Error Handling:**
  - Handles error conditions with proper exit codes.
  - Checks return codes of system calls.
