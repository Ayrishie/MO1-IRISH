# Multitasking OS Emulator (C++)

## Author
BALDEROSA, CHAN, RAMOS, TAMSE

---

## Overview
A console-based OS emulator that demonstrates:

- **Command-driven shell**
  - `initialize` (reads `config.txt`)
  - `scheduler-start`, `scheduler-stop`, `scheduler-test`
  - `screen -ls`, `screen -r <name>`, `screen -s <name> <memory>`, `screen -c <name> [memory] "<instructions>"`
  - `process-smi` (summary view like nvidia-smi)
  - `vmstat` (detailed memory/CPU/page stats)
  - `report-util`, `clear`, `exit`
- **Schedulers:** First-Come First-Served (FCFS) and Round-Robin (RR)
- **Demand paging memory manager**
  - Configurable **max overall memory** and **frame/page size**
  - Global **LRU** page replacement
  - **Backing store** persisted as `csopesy-backing-store.txt`
  - **Page-ins / Page-outs** counters
- **Process model**
  - PID, name, current core, start time, progress
  - Instruction set: `DECLARE`, `ADD`, `SUBTRACT`, `SLEEP`, `FOR`, `READ`, `WRITE`, `PRINT`
  - **Symbol table**: 64-byte segment (32 × `uint16`) **paged on virtual page 0**
  - **Access-violation** handling with per-process crash banner in `screen -r`
- **CPU accounting aligned with spec**
  - An **Active** tick is counted **only when an instruction completes**
  - Page-fault stalls **do not burn quantum** and are counted as **Idle**

---

## Entry Point
- **File:** `src/main.cpp`  
- **Function:** `int main(int argc, char** argv)`

---

## Project Structure (key files)
- `Console.cpp/.h` – shell, parsing, command handlers, output formatting  
- `RRScheduler.cpp/.h`, `FCFSScheduler.cpp/.h`, `Scheduler.h` – CPU workers & algorithms  
- `Process.cpp/.h` – process state, instruction execution, per-process log  
- `Instruction.h` – instruction set & **ProcessContext** (paging + symbol table on page 0)  
- `MemoryManager.cpp/.h` – frames, LRU, word I/O (`readWord`, `writeWord`), backing store I/O  
- `Config.txt` – configuration consumed by `initialize`  
- `processesLogs/` – per-process textual logs  
- `csopesy-backing-store.txt` – **eviction log** / simulated swap

---

## Prerequisites
- A C++17-capable compiler  
  - **Windows (recommended):** Visual Studio 2019+ (MSVC)  
  - **Linux / WSL / MSYS2:** `g++` (with `-pthread`)
- Terminal/console window (program is interactive)

---

## Building & Running (Visual Studio 2022)

1. **Open the solution**  
   Double-click `MO1-IRISH.sln` (or the provided solution file). VS opens with the project loaded.

2. **Select configuration**  
   Set the top toolbar to **Debug** and **x64**.

3. **Build & run**  
   Click **Local Windows Debugger** (▶) or press **F5**.  
   An integrated console starts the emulator.

> **Tip:** The program looks for `config.txt`. Place it next to the executable or in the project root if running from VS.

---
