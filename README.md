# Tinky-Winkey Keylogger
## Description

This project is an educational keylogger developed in C/C++ for Windows. It is designed to capture keyboard keystrokes and active window changes while running as a Windows service. **The project is for educational purposes only and should not be used for malicious intent.**

The Windows service associated with the keylogger is called `tinky` and runs a program called `winkey` in the background, which records keystrokes and information about the active window to a log file.

## Features

- Captures keyboard keystrokes with the current keyboard mapping.
- Logs keystrokes, active window name, and processes.
- Tracks window changes (including tab changes for certain browsers).
- Functions as a Windows service (`tinky`), allowing background execution.
- The service can be installed, started, stopped, and removed using simple commands.

## Project Structure

- **svc.exe**: Windows service that controls the keylogger.
- **winkey.exe**: The keylogger that records keystrokes and window changes.
- **src/**: Contains the project's source files in C/C++.
- **C:\\winkey.log**: Log file where keystrokes and window information are recorded.

## Operation

The project operates in two main parts:

1. **The Service (`tinky`)**:
	- Manages the launch and termination of the keylogger.
	- Once started, it impersonates a SYSTEM token and launches `winkey.exe`.

2. **The Keylogger (`winkey`)**:
	- Records each keystroke and detects active window changes.
	- Keystrokes and window names are recorded in the `C:\\winkey.log` file with a timestamp.

## Installation and Usage

### 1. Compilation

To compile the project, you need to use **NMAKE** and the **CL** compiler from Microsoft Visual Studio with the `/Wall` and `/WX` flags for warnings and errors.

### 2. Service Installation

After compiling the project, you can install and start the tinky service using the following commands from an elevated command prompt:

```bash
svc.exe install
svc.exe start
```

3. Service Stop and Removal

To stop and remove the service, use the following commands:

```bash
svc.exe stop
svc.exe delete
```

4. Log File

All keystrokes and active window information are recorded in the following file:

```
C:\\winkey.log
```

The log file contains entries in the following format:

```
[DD-MM-YYYY HH:MM:SS] - Active Window: <Active Window Name>
[DD-MM-YYYY HH:MM:SS] - Key: <Key Code> (<Character>)
```