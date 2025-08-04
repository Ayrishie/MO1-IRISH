// Console.cpp — includes for everything you use in here
#include "Console.h"          // your class declaration
#include <cstdlib>            // system()
#include <iostream>           // cout, cerr
#include <fstream>            // ifstream, ofstream
#include <sstream>            // stringstream
#include <chrono>             // system_clock
#include <iomanip>            // put_time
#include <algorithm>          // remove
#include <random>             // random_device, mt19937, uniform_int_distribution

// filesystem support (works both on GCC/Clang and MSVC)
#if defined(_MSC_VER) && defined(_MSVC_LANG) && _MSVC_LANG >= 201703L
#include <filesystem>
namespace fs = std::filesystem;
#elif __cplusplus >= 201703L
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "Requires C++17 or later for filesystem support"
#endif


// check for OS Platform and define a single MACRO for terminal clearing
#ifdef _WIN32
#define SYSCLEAR system("cls")
#else
#define SYSCLEAR system("clear")
#endif


using namespace std;


//Handling File of process
const std::string logDir = "processesLogs";


Console::Console() {

}

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &now_time);  // Windows-safe version
    std::stringstream ss;
    ss << std::put_time(&tm, "%m/%d/%Y %I:%M:%S %p");
    return ss.str();
}

void Console::header() {
    string blk = "\033[47m  \033[0m";

    cout << " " + blk + blk + blk + "  " + blk + blk + blk + "   " + blk + blk + blk + "   " + blk + blk + blk + "   " + blk + blk + blk + "  " + blk + blk + blk + "  " + blk + "  " + blk << endl;
    cout << blk + "   " + blk + "  " + blk + "      " + blk + "    " + blk + "  " + blk + "   " + blk + "  " + blk + "      " + blk + "      " + blk + "  " + blk << endl;
    cout << blk + "       " + blk + blk + blk + "  " + blk + "    " + blk + "  " + blk + "   " + blk + "  " + blk + blk + blk + "  " + blk + blk + blk + "   " + blk + blk << endl;
    cout << blk + "   " + blk + "      " + blk + "  " + blk + "    " + blk + "  " + blk + blk + blk + "   " + blk + "          " + blk + "    " + blk << endl;
    cout << " " + blk + blk + blk + "  " + blk + blk + blk + "   " + blk + blk + blk + "   " + blk + "       " + blk + blk + blk + "  " + blk + blk + blk + "    " + blk << endl;

    cout << "----------Operating Systems Command Line Emulator----------" << endl;
    cout << "===========================================================\n" << endl;
    menu();
}

void Console::menu()
{
    cout << "Available commands:" << endl;
    cout << "  initialize     - Initialize system" << endl;
    cout << "  screen -c <name> <memory> \"<instructions>\" - Create a new process with custom instructions" << endl;
    cout << "  screen -r <name> - Display screen for a process" << endl;
    cout << "  screen -s <name> <process_memory_size> - Create a new screen for a process" << endl;
    cout << "  screen -ls       - List system utilization and processes" << endl;
    cout << "  scheduler-start - Start scheduler" << endl;
    cout << "  scheduler-stop - Stop scheduler" << endl;
    cout << "  report-util    - Report system utilization" << endl;
    cout << "  process-smi       - Show summarized memory and CPU usage" << endl;
    cout << "  vmstat            - Show detailed virtual memory stats" << endl;
    cout << "  clear          - Clear screen" << endl;
    cout << "  exit           - Exit application\n\n" << endl;
}

void Console::start() {

    string exitInput = "exit";  

    while (this->userInput != exitInput) {
        cout << "Enter a command: ";
        getline(cin, this->userInput);
        parseInput(this->userInput);
    }
}

void Console::initialize() {

    if (schedulerRunning) {
        std::cerr << "\033[31m";
        std::cerr << "Error: Cannot re-initialize system while scheduler is running.\n";
        std::cerr << "Stop the scheduler first using 'scheduler-stop'.\n";
        std::cerr << "\033[0m";
        return;
    }

    clear();

    // -------------------------
    // Defaults (kept on purpose; config.txt can override these)
    // -------------------------
    cpuCount = 1;
    timeQuantum = 0;
    batchProcessFreq = 1;
    minInstructions = 1000;
    maxInstructions = 2000;
    delayPerExecution = 0;
    schedulerType = "fcfs";

    // Sensible memory defaults (powers of two)
    maxOverallMem = 16 * 1024; // 16384 bytes
    memPerFrame = 64;        // bytes per frame
    minMemPerProc = 64;        // bytes
    maxMemPerProc = 512;       // bytes

    // Ensure the log directory exists (create if missing)
    if (!fs::exists(logDir)) {
        fs::create_directory(logDir);
    }
    else if (fs::is_directory(logDir)) {
        // Clean up old .txt logs
        for (const auto& entry : fs::directory_iterator(logDir)) {
            if (entry.path().extension() == ".txt") {
                fs::remove(entry);
            }
        }
    }

    // -------------------------
    // Read config.txt (overrides defaults)
    // -------------------------
    std::ifstream config("config.txt");
    if (!config.is_open()) {
        std::cerr << "Error: Could not open config.txt\n";
        return;
    }

    std::string key, value;
    while (config >> key >> value) {
        // strip quotes if present
        value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());

        if (key == "scheduler")          schedulerType = value;
        else if (key == "num-cpu")            cpuCount = std::stoi(value);
        else if (key == "quantum-cycles")     timeQuantum = std::stoi(value);
        else if (key == "batch-process-freq") {
            batchProcessFreq = std::stoi(value);
            if (batchProcessFreq < 1) {
                std::cerr << "\033[31mError: batch-process-freq must be at least 1\033[0m\n";
                return;
            }
        }
        else if (key == "min-ins")            minInstructions = std::stoi(value);
        else if (key == "max-ins")            maxInstructions = std::stoi(value);
        else if (key == "delay-per-exec")     delayPerExecution = std::stoi(value);
        else if (key == "max-overall-mem")    maxOverallMem = std::stoi(value);
        else if (key == "mem-per-frame")      memPerFrame = std::stoi(value);
        else if (key == "min-mem-per-proc")   minMemPerProc = std::stoi(value);
        else if (key == "max-mem-per-proc")   maxMemPerProc = std::stoi(value);
        else {
            std::cout << "Warning: unknown key \"" << key << "\" skipped\n";
        }
    }





    // -------------------------
    // Sanity / safety fixups (avoid RNG assert and invalid frames)
    // -------------------------
 
    // Ensure min/max process memory are a valid range even if config.txt is wrong.
    if (minMemPerProc > maxMemPerProc) std::swap(minMemPerProc, maxMemPerProc);

    // Keep them strictly positive (uniform_int requires lo <= hi and both valid ints).
    if (minMemPerProc < 1) minMemPerProc = 1;
    if (maxMemPerProc < minMemPerProc) maxMemPerProc = minMemPerProc;


    if (memPerFrame <= 0) {
        std::cerr << "\033[31mError: mem-per-frame must be > 0\033[0m\n";
        return;
    }

    // Validate full memory configuration
    if (!memoryCheck(maxOverallMem, memPerFrame, minMemPerProc, maxMemPerProc)) {
        return;
    }

    // -------------------------
    // Initialize paging allocator and schedulers
    // -------------------------
    pagingAllocator = std::make_unique<PagingAllocator>(maxOverallMem, memPerFrame);

    std::cout << "\033[32m";
    std::cout << "===============================\n";
    std::cout << "|    SYSTEM INITIALIZATION    |\n";
    std::cout << "===============================\n";
    std::cout << "\033[0m";

    std::cout << "\033[36m";
    std::cout << "Scheduler: " << schedulerType << "\n";
    std::cout << "CPU Count: " << cpuCount << "\n";
    if (schedulerType == "rr") {
        std::cout << "Quantum: " << timeQuantum << "\n";
    }
    else {
        std::cout << "Quantum: N/A (FCFS)\n";
    }
    std::cout << "Batch Frequency: " << batchProcessFreq << " ticks\n";
    std::cout << "Instructions: " << minInstructions << " to " << maxInstructions << "\n";
    std::cout << "Delay per Exec: " << delayPerExecution << "ms\n";
    std::cout << "Max overall Mem: " << maxOverallMem << "bytes\n";
    std::cout << "Mem per Frame " << memPerFrame << "bytes\n";
    std::cout << "Min mem per Proc " << minMemPerProc << "bytes\n";
    std::cout << "Max mem per Proc " << maxMemPerProc << "bytes\n";
    std::cout << "\033[0m";

    processes.clear();
    schedulerRunning = false;

    if (schedulerType == "rr") {
        rrScheduler = std::make_unique<RRScheduler>(
            cpuCount, timeQuantum, delayPerExecution, pagingAllocator.get());
    }
    else if (schedulerType == "fcfs") {
        fcfsScheduler = std::make_unique<FCFSScheduler>(
            cpuCount, delayPerExecution, pagingAllocator.get());
    }
    else {
        std::cerr << "Error: unknown scheduler type '" << schedulerType << "' in config.txt\n";
    }
}


// UPDATED FOR NEW CONFIG PARAMETERS
bool Console::memoryCheck(int maxMem, int frameSize, int minMemPerProc, int maxMemPerProc) {
    auto isPowerOfTwo = [](int x) {
        return x > 0 && (x & (x - 1)) == 0;
        };

    if (!isPowerOfTwo(maxMem) || !isPowerOfTwo(frameSize)
        || !isPowerOfTwo(minMemPerProc) || !isPowerOfTwo(maxMemPerProc)) {
        std::cerr << "\033[31mError: Memory values must be powers of 2\033[0m\n";
        return false;
    }
    if (maxMem < 64 || maxMem > 65536 ||
        frameSize < 16 || frameSize > 65536 || // Change this to 64 after
        minMemPerProc < 64 || minMemPerProc > 65536 ||
        maxMemPerProc < 64 || maxMemPerProc > 65536) {
        std::cerr << "\033[31mError: Memory values must be in range [64, 65536]\033[0m\n";
        return false;
    }

    if (maxMem % frameSize != 0) {
        std::cerr << "\033[31mError: maxOverallMem must be divisible by memPerFrame\033[0m\n";
        return false;
    }

    if (minMemPerProc > maxMem || maxMemPerProc > maxMem) {
        std::cerr << "\033[31mError: memPerProc exceeds maxOverallMem\033[0m\n";
        return false;
    }

    return true;
}



void Console::schedulerStart() {
    clear();

    if (!fcfsScheduler && !rrScheduler) {
        std::cerr << "\033[31mError: Scheduler not initialized. Please run initialize first.\033[0m\n";
        return;
    }
    if (schedulerRunning) {
        std::cout << "\033[33mWarning: Scheduler is already running!\033[0m\n";
        return;
    }
    if (schedulerThread.joinable()) {
        std::cout << "\033[33mWarning: Scheduler thread is already running!\033[0m\n";
        return;
    }

    // Clamp unsafe or nonsensical values from config
    const int safeDelayMs = std::max(1, delayPerExecution);        // never 0ms
    const int safeBatchFreq = std::max(1, batchProcessFreq);       // never 0
    const int safeMinIns = std::max(1, minInstructions);
    const int safeMaxIns = std::max(safeMinIns, maxInstructions);  // ensure max>=min

    schedulerRunning = true;

    // Header depending on scheduler type
    std::cout << "\033[32m";
    std::cout << "=====================================\n";
    std::cout << "|      SCHEDULER START (" << (schedulerType == "rr" ? "RR" : "FCFS") << ")       |\n";
    std::cout << "=====================================\n";
    std::cout << "\033[0m";

    std::cout << "\033[36m";
    if (schedulerType == "rr") {
        std::cout << "Starting Round Robin scheduler with " << cpuCount
            << " CPUs and time quantum " << timeQuantum << "\n";
        rrScheduler->start();
    }
    else {
        std::cout << "Starting FCFS scheduler with " << cpuCount << " CPUs\n";
        fcfsScheduler->start();
    }
    std::cout << "Process will be generated every " << safeBatchFreq << " ticks\n";
    std::cout << "\033[0m";

    // Feed thread: periodically create processes
    schedulerThread = std::thread([this, safeDelayMs, safeBatchFreq, safeMinIns, safeMaxIns]() {
        int tick = 0;
        int quantumCycle = 0;

        // Simple back-pressure so we don't create unbounded processes
        constexpr size_t ACTIVE_CAP = 200; // tune as you like

        while (schedulerRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(safeDelayMs));
            ++tick;

            if (tick % safeBatchFreq == 0) {
                // Count currently active (not finished) processes
                size_t active = 0;
                {
                    std::lock_guard<std::mutex> lock(processesMutex);
                    for (const auto& p : processes) {
                        if (!p->isFinished()) ++active;
                    }
                }
                if (active >= ACTIVE_CAP) {
                    // Too many in-flight; skip creating this tick
                    continue;
                }

                // Create a new process
                std::ostringstream nameStream;
                nameStream << "p" << std::setfill('0') << std::setw(2) << ++pidCounter;
                const std::string name = nameStream.str();

                // #instructions in [safeMinIns, safeMaxIns]
                const int commands =
                    safeMinIns + (std::rand() % (safeMaxIns - safeMinIns + 1));

                auto process = std::make_shared<Process>(name, commands, generateProcessSize());
                {
                    std::lock_guard<std::mutex> lock(processesMutex);
                    processes.push_back(process);
                }

                if (schedulerType == "rr") {
                    rrScheduler->enqueueProcess(process);
                }
                else {
                    fcfsScheduler->addProcess(process);
                }
            }

            // Optional snapshot cadence if you later hook it up
            if (timeQuantum > 0 && (tick % timeQuantum) == 0) {
                ++quantumCycle;
            }
        }
        });

    isInitialized = true;
    std::cout << "\033[36mScheduler started successfully.\n\n\033[0m";
}


void Console::createProcessFromCommand(const std::string& procName, int procMem) {
    if (!validateProcessCreation(procName, procMem)) {
        return;
    }

    int commands = minInstructions + (rand() % (maxInstructions - minInstructions + 1));
    size_t memory = procMem;
    auto process = std::make_shared<Process>(procName, commands, memory);
    {
        std::lock_guard<std::mutex> lock(processesMutex);
        processes.push_back(process);
    }

    {
        if (schedulerType == "rr") {
            rrScheduler->enqueueProcess(process);
        }
        else {
            fcfsScheduler->addProcess(process);
        }

        pidCounter++;
        std::cout << "\033[32mCreated process \"" << procName << "\" with " << commands
            << " instructions and " << memory << " bytes of memory.\033[0m\n";
    }
}

void Console::attachToProcessScreen(const std::string& procName) {
    bool found = false;

    {
        std::lock_guard<std::mutex> lock(processesMutex);
        for (const auto& process : processes) {
            if (process->getName() == procName) {
                found = true;
                break;
            }
        }
    }
    if (!found) {
        std::cerr << "\033[31mError: Process \"" << procName << "\" not found.\033[0m\n";
        return;
    }

    showProcessScreen(procName);
}



void Console::screen() {
    if (userInput.find("-ls") != string::npos) {
        listProcesses();
    }
    else {
        cout << "Usage: screen -ls (to list processes)\n";
    }
}

void Console::showProcessScreen(const std::string& procName) {
    // Case-insensitive lookup
    auto toUpper = [&](std::string s) {
        for (auto& c : s) c = static_cast<char>(::toupper(c));
        return s;
        };
    std::string target = toUpper(procName);
    std::shared_ptr<Process> procPtr;

    for (auto& p : processes) {
        if (toUpper(p->name) == target) {
            procPtr = p;
            break;
        }
    }
    if (!procPtr) {
        std::cout << "\033[31mProcess not found: " << procName << "\033[0m\n";
        return;
    }

    // Clear screen
    SYSCLEAR;

    // Main loop
    while (true) {
        // A) Header
        std::cout << "Process name: " << procPtr->name << "\n";
        std::cout << "ID:           " << procPtr->process_id << "\n";
        std::cout << "Memory:       " << procPtr->processMemory << " bytes\n";

        // B) Logs
        std::cout << "Logs:\n";
        for (auto& line : procPtr->getRecentOutputs()) {
            std::cout << line << "\n";
        }
        std::cout << "\n";
        procPtr->clearRecentOutputs();

        // C) Execution state
        std::cout << "Current instruction line: "
            << procPtr->getCurrentInstructionLine() << "\n";
        std::cout << "Lines of code:           "
            << procPtr->total_commands << "\n";

        // print status (e.g. “Core 0” or “Finished”)
        std::cout << "Status:                  "
            << procPtr->getStatus() << "\n";
        // print the next instruction itself, if not finished
        if (!procPtr->isFinished()) {
            auto& instrs = procPtr->getInstructions();
            int idx = procPtr->getCurrentInstructionLine();
            std::cout << "Current instruction:     "
                << (idx < (int)instrs.size()
                    ? instrs[idx]->toString()
                    : std::string("<none>"))
                << "\n";

        }
        if (procPtr->isFinished()) {
            std::cout << "Finished!\n\n";
        }

        // D) Prompt
        while (true) {
            std::cout << "\n";
            std::cout << "(type 'exit' to return or 'process-smi' to refresh) > ";
            std::string cmd;
            std::getline(std::cin, cmd);

            // strip trailing CR/whitespace (so "exit\r" or "exit " still matches)
            while (!cmd.empty() && (cmd.back() == '\r' || std::isspace((unsigned char)cmd.back()))) {
                cmd.pop_back();
            }

            // make uppercase so it's case‐insensitive
            std::transform(cmd.begin(), cmd.end(), cmd.begin(),
                [](unsigned char c) { return std::toupper(c); });

            if (cmd == "EXIT") {
                // return out of showProcessScreen entirely

                SYSCLEAR;
                
                header();
                return;
            }
            else if (cmd == "PROCESS-SMI") {
                // redraw the same screen
                break;
            }
            else {
                std::cout << "Unknown command: " << cmd << "\n";
                // and loop back without leaving
                continue;
            }
        }

        // if we got here, user typed PROCESS-SMI, so we just fall out of
        // the prompt‐loop and the outer while(true) at the top of
        // showProcessScreen will re-draw automatically.


    }
}




void Console::printUtilization(std::ostream* out) const {
    // choose whether we're writing to cout or a file
    std::ostream& o = out ? *out : std::cout;

    // count how many are still active
    int activeCount = 0;
    for (auto& p : processes) {
        if (!p->isFinished()) ++activeCount;
    }

    // cores used = min(activeCount, cpuCount)
    int coresUsed = std::min(activeCount, cpuCount);
    int coresAvail = cpuCount - coresUsed;

    // utilization percentage
    double util = (double)coresUsed / (double)cpuCount * 100.0;

    // print exactly like the spec screenshot
    o << "CPU utilization: "
        << std::fixed << std::setprecision(0)
        << util << "%\n";
    o << "Cores used:       " << coresUsed << "\n";
    o << "Cores available:  " << coresAvail << "\n";
    o << "----------------------------------------\n";
}



void Console::listProcesses() {
    system("cls");// remove past DELETE

    if (processes.empty()) {
        clear();
        std::cout << "\033[93m"; // Light yellow
        std::cout << "No processes available.\n";
        std::cout << "\033[0m";
        return;
    }

    header();

    // Green header and lines
    std::cout << "\033[32m";
    std::cout << "\n\nList of processes (Refreshed at " << getCurrentTime() << "):\n";
    std::cout << "=====================================================================\n";
    std::cout << "\033[0m";

    // CPU summary: light yellow (optional, you can keep default if preferred)
    std::cout << "\033[93m";
    printUtilization(nullptr);
    std::cout << "\033[0m";

    // Column headers: cyan
    std::cout << "\033[36m";
    std::cout << std::left << std::setw(10) << "Name"
        << std::setw(25) << "Start Time"
        << std::setw(15) << "Status"
        << std::setw(15) << "Progress"
        << std::setw(10) << "Memory"
        << "Core Assignment\n";
    std::cout << "\033[0m";

    // Green divider
    std::cout << "\033[32m";
    std::cout << "=====================================================================\n\n";
    std::cout << "\033[0m";

    // Light yellow for process rows
    std::cout << "\033[93m";
    if (schedulerType == "rr") {
        rrScheduler->displayProcesses(); // implemented RR version for displayProcesses()
    }
    else {
        fcfsScheduler->displayProcesses();
    }

    std::cout << "\033[0m";

    std::cout << "\033[32m";
    std::cout << "=====================================================================\n";
    std::cout << "\033[0m";
}



void Console::displayContinuousUpdates() {
    while (schedulerRunning) {
        header();
        listProcesses();
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void Console::schedulerTest() {
    if (processes.empty()) {
        std::cout << "No processes to schedule. Use 'scheduler-start' first.\n";
        return;
    }

    // Clear screen and show test header
    clear();
    std::cout << "========================================\n";
    std::cout << "|        SCHEDULER TEST MODE          |\n";
    std::cout << "========================================\n";
    std::cout << (schedulerType == "rr"
        ? "Round Robin | CPUs: " + std::to_string(cpuCount) + " | Quantum: " + std::to_string(timeQuantum) + "\n"
        : "FCFS | CPUs: " + std::to_string(cpuCount) + "\n");
    std::cout << "----------------------------------------\n";
    std::cout << "Valid commands:\n";
    std::cout << "  stop      - Terminate all processes\n";
    std::cout << "  pause     - Pause execution\n";
    std::cout << "  continue  - Resume execution\n";
    std::cout << "  exit-test - Exit test mode\n";
    std::cout << "----------------------------------------\n\n";

    schedulerRunning = true;
    std::atomic<bool> paused{ false };
    std::atomic<bool> shouldStop{ false };
    std::atomic<bool> exitTest{ false };

    // Input handling thread
    std::thread inputThread([&]() {
        std::string cmd;
        while (schedulerRunning && !exitTest) {
            std::cout << "test> ";
            std::getline(std::cin, cmd);

            // Convert to lowercase and trim whitespace
            cmd.erase(remove_if(cmd.begin(), cmd.end(), ::isspace), cmd.end());
            transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

            if (cmd == "stop") {
                shouldStop = true;
                schedulerRunning = false;
                std::cout << "\033[31mTerminating all processes...\033[0m\n";
            }
            else if (cmd == "pause") {
                paused = true;
                std::cout << "\033[33mTest paused. Type 'continue' to resume.\033[0m\n";
            }
            else if (cmd == "continue") {
                paused = false;
                std::cout << "\033[32mResuming test...\033[0m\n";
            }
            else if (cmd == "exit-test") {
                exitTest = true;
                std::cout << "\033[36mExiting test mode...\033[0m\n";
            }
            else {
                std::cout << "\033[31mInvalid command in test mode.\033[0m Valid commands: "
                    << "\033[36mstop, pause, continue, exit-test\033[0m\n";
            }

        }
        });

    // Scheduler thread
    std::thread schedulerThread([this, &paused, &shouldStop, &exitTest]() {
        if (schedulerType == "rr") {
            rrScheduler->start();
        }
        else {
            fcfsScheduler->start();
        }

        while (schedulerRunning && !exitTest) {
            if (paused) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            // Check if all processes are finished
            bool allFinished = true;
            {
                std::lock_guard<std::mutex> lock(processesMutex);
                for (const auto& p : processes) {
                    if (!p->isFinished()) {
                        allFinished = false;
                        break;
                    }
                }
            }

            if (allFinished) {
                std::cout << "\nAll processes completed.\n";
                schedulerRunning = false;
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Cleanup
        if (shouldStop) {
            // Force terminate all processes
            std::lock_guard<std::mutex> lock(processesMutex);
            for (auto& p : processes) {
                if (!p->isFinished()) {
                    p->core_id = -1;
                    p->executed_commands = p->total_commands; // Mark as finished
                }
            }
        }

        if (schedulerType == "rr") {
            rrScheduler->stop();
        }
        else {
            fcfsScheduler->stop();
        }
        });

    // Display thread
    std::thread displayThread([this, &paused, &exitTest]() {
        while (schedulerRunning && !exitTest) {
            if (paused) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            system("cls");
            header();
            listProcesses();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        });

    // Wait for threads
    if (schedulerThread.joinable()) schedulerThread.join();
    if (displayThread.joinable()) displayThread.join();
    if (inputThread.joinable()) inputThread.join();

    // Clear and return to main menu
    if (exitTest) {
        clear();
        header();
    }
    else {
        std::cout << "\nTest completed. Press any key to continue...\n";
        std::cin.ignore();
        clear();
        header();
    }
}

void Console::schedulerStop() {
    if (schedulerRunning) {
        schedulerRunning = false; // Stop adding new processes only

        if (schedulerThread.joinable()) {
            schedulerThread.join();
        }
        std::cout << "\033[31m";
        std::cout << "==============================\n";
        std::cout << "|    PROCESS FEED STOPPED    |\n";
        std::cout << "==============================\n";
        std::cout << "\033[0m";
    }
    else {
        std::cout << "Scheduler is not currently accepting new processes.\n";
    }
}


void Console::reportUtil() {
    std::ofstream file("csopesy-log.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Could not open csopesy-log.txt for writing.\n";
        return;
    }

    if (processes.empty()) {
        file << "No processes available.\n";
        file.close();
        return;
    }

    // Timestamp
    file << "List of processes (Saved at " << getCurrentTime() << "):\n";
    file << "=====================================================================\n";

    printUtilization(&file);

    // Column headers
    file << std::left << std::setw(10) << "Name"
        << std::setw(25) << "Start Time"
        << std::setw(15) << "Status"
        << std::setw(15) << "Progress"
        << std::setw(10) << "Memory"
        << "Core Assignment\n";

    file << "=====================================================================\n\n";

    if (schedulerType == "rr") {
        rrScheduler->displayProcesses(file); // implemented displayProcess(file) version for RR
        // for uniformity and avoid access to destroyed process objects
    }
    else {
        fcfsScheduler->displayProcesses(file);  // also overloaded
    }


    file << "=====================================================================\n";
    file.close();

    std::cout << "Report generated at ./Debug/csopesy-log.txt!\n\n";
}


void Console::clear() {
    system("cls");
    header();
}




void Console::parseInput(std::string userInput) {
    std::string originalInput = userInput;

    std::string loweredInput = userInput;
    std::transform(loweredInput.begin(), loweredInput.end(), loweredInput.begin(), ::tolower);

    std::istringstream iss(loweredInput);
    std::vector<std::string> args;
    std::string token;

    while (iss >> token) {
        args.push_back(token);
    }

    if (args.empty()) return;

    if (args[0] == "exit") {
        std::_Exit(EXIT_SUCCESS);
    }
    else if (args[0] == "initialize") {
        initialize();
    }
    else if (args[0] == "screen" && args.size() == 2 && args[1] == "-ls") {
        screen();
    }
    else if (args[0] == "scheduler-start") {
        schedulerStart();
    }
    else if (args[0] == "scheduler-test") {
        schedulerTest();
    }
    else if (args[0] == "scheduler-stop") {
        schedulerStop();
    }
    else if (args[0] == "report-util") {
        reportUtil();
    }
    else if (args[0] == "process-smi") {
        // show summarized CPU/memory usage (like nvidia-smi, but for your emulator)
        clear();                    // optional, redraw header
        printUtilization(nullptr);  // nullptr -> print to std::cout
    }
    else if (args[0] == "vmstat") {
        // optional helper you listed in the menu
        if (pagingAllocator) {
            pagingAllocator->visualizeMemory();
        }
        else {
            std::cout << "Paging allocator not initialized. Run 'initialize' first.\n";
        }
    }
    else if (args[0] == "clear") {
        clear();
    }
    // NEW: screen -s <process_name> <process_memory_size>
    else if (args[0] == "screen" && args.size() == 4 && args[1] == "-s") {
        std::string procName = args[2];
        try {
            int procMem = std::stoi(args[3]);
            createProcessFromCommand(procName, procMem);
        }
        catch (...) {
            std::cerr << "Error: Memory size must be a valid integer.\n";
        }
    }
    // screen -r <process_name>
    else if (args[0] == "screen" && args.size() == 3 && args[1] == "-r") {
        std::string procName = args[2];
        attachToProcessScreen(procName);
    }
    else if (args[0] == "screen" && args.size() >= 5 && args[1] == "-c") {
        std::string procName = args[2];

        int memSize;
        try {
            memSize = std::stoi(args[3]);
        }
        catch (...) {
            std::cerr << "\033[31mError: Memory size must be a valid integer.\033[0m\n";
            return;
        }

        size_t firstQuote = originalInput.find('"');
        size_t lastQuote = originalInput.rfind('"');
        if (firstQuote == std::string::npos || lastQuote == std::string::npos || lastQuote <= firstQuote) {
            std::cerr << "\033[31mError: Instruction string must be wrapped in double quotes.\033[0m\n";
            return;
        }

        std::string instructionStr = originalInput.substr(firstQuote + 1, lastQuote - firstQuote - 1);

        createProcessWithInstructions(procName, memSize, instructionStr);
    }
    else {
        std::cout << "Unknown command: " << userInput << "\n";
    }
}


// New

bool Console::isValidProcessMemory(int procMem) {
    auto isPowerOfTwo = [](int x) {
        return x > 0 && (x & (x - 1)) == 0;
        };

    if (!isPowerOfTwo(procMem)) {
        std::cerr << "\033[31mInvalid memory allocation: Not a power of 2\033[0m\n";
        return false;
    }

    if (procMem < 64 || procMem > 65536) {
        std::cerr << "\033[31mInvalid memory allocation: Must be in range [64, 65536]\033[0m\n";
        return false;
    }

    return true;
}

bool Console::validateProcessCreation(const std::string& procName, int procMem) {
    clear();
    if (procName.empty()) {
        std::cout << "\033[33mWarning: Process name required.\033[0m\n";
        return false;
    }

    if (!isValidProcessMemory(procMem)) {
        return false;
    }

    if (!fcfsScheduler && !rrScheduler) {
        std::cerr << "\033[31mError: Scheduler not initialized. Please run initialize first.\033[0m\n";
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(processesMutex);
        for (const auto& p : processes) {
            if (p->getName() == procName) {
                std::cout << "\033[33mWarning: Process \"" << procName << "\" already exists.\033[0m\n";
                return false;
            }
        }
    }

    return true;
}

// NEW --GENERRATE PROCESS MEMORY SIZE BASED ON MEMORY RANGE CONFIG
int Console::generateProcessSize() {
    // guard values read from config
    int lo = std::min(minMemPerProc, maxMemPerProc);
    int hi = std::max(minMemPerProc, maxMemPerProc);

    // enforce valid positive bounds
    if (hi < 64)  hi = 64;
    if (lo < 64)  lo = 64;
    if (hi < lo)  hi = lo;

    static thread_local std::mt19937 gen{ std::random_device{}() };
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(gen);
}
// ---- helpers for parsing -----------------------------------------------------

static inline void trim_in_place(std::string& s) {
    auto issp = [](unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!s.empty() && issp((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && issp((unsigned char)s.back()))  s.pop_back();
}


static inline std::string cleanName(std::string s) {
    trim_in_place(s);
    // strip a trailing semicolon if any
    if (!s.empty() && s.back() == ';') s.pop_back();
    trim_in_place(s);
    return s;
}

static inline uint16_t parseU16(std::string s) {
    s = cleanName(std::move(s));
    int base = 10;
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
        base = 16; s = s.substr(2);
    }
    return static_cast<uint16_t>(std::stoul(s, nullptr, base));
}



static inline bool isDecimalNumber(const std::string& s) {
    if (s.empty()) return false;
    for (unsigned char c : s) if (!std::isdigit(c)) return false;
    return true;
}


static inline uint16_t parseAddress(const std::string& s) {
    // If it looks hex (0x.. or has A..F), parse as hex; else decimal.
    bool looksHex = (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'));
    if (!looksHex) {
        for (unsigned char c : s) {
            if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) { looksHex = true; break; }
        }
    }
    try {
        return static_cast<uint16_t>(
            std::stoul(s, nullptr, looksHex ? 16 : 10)
            );
    }
    catch (...) {
        return 0;
    }
}
// -----------------------------------------------------------------------------


void Console::createProcessWithInstructions(const std::string& procName,
    int procMem,
    const std::string& instructionStr)
{
    // Validate process creation first
    if (!validateProcessCreation(procName, procMem)) return;

    std::cout << "\033[36m[INFO] Creating process: " << procName << "\033[0m\n";
    std::cout << "\033[36m[INFO] Memory size: " << procMem << " bytes\033[0m\n";
    std::cout << "\033[36m[INFO] Raw instruction input: " << instructionStr << "\033[0m\n";

    std::vector<std::shared_ptr<Instruction>> parsedInstructions;
    bool hasInvalidInstruction = false;

    // --- 1) Split the script on ';' but keep quoted parts intact ---
    std::vector<std::string> instructionLines;
    {
        std::string cur;
        bool inQuote = false;
        for (char ch : instructionStr) {
            if (ch == '"') inQuote = !inQuote;
            if (ch == ';' && !inQuote) {
                trim_in_place(cur);
                if (!cur.empty()) instructionLines.push_back(cur);
                cur.clear();
            }
            else {
                cur.push_back(ch);
            }
        }
        trim_in_place(cur);
        if (!cur.empty()) instructionLines.push_back(cur);
    }

    std::cout << "\033[36m[INFO] Parsed instructions:\033[0m\n";
    for (std::string line : instructionLines) {
        std::cout << " - " << line << "\n";
        trim_in_place(line);
        if (line.empty()) continue;

        // Grab leading keyword (letters only), keep the rest as args
        size_t i = 0;
        while (i < line.size() && std::isalpha(static_cast<unsigned char>(line[i]))) ++i;
        std::string keyword = line.substr(0, i);
        std::string args = (i < line.size() ? line.substr(i) : std::string());

        // lower-case keyword for matching
        std::transform(keyword.begin(), keyword.end(), keyword.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });

        // ========== DECLARE ==========
        if (keyword == "declare") {
            std::stringstream ss(args);
            std::string var, valStr;
            ss >> var >> valStr;
            trim_in_place(var);
            trim_in_place(valStr);
            if (var.empty() || valStr.empty() || !isDecimalNumber(valStr)) {
                std::cerr << "\033[31m[ERROR] Bad DECLARE: " << line << "\033[0m\n";
                hasInvalidInstruction = true;
                continue;
            }
            uint16_t value = parseU16(valStr);
            parsedInstructions.push_back(std::make_shared<DeclareInstruction>(var, value));
        }
        // ========== ADD ==========
        else if (keyword == "add") {
            std::stringstream ss(args);
            std::string target, a1, a2;
            ss >> target >> a1 >> a2;
            trim_in_place(target); trim_in_place(a1); trim_in_place(a2);
            if (target.empty() || a1.empty() || a2.empty()) {
                std::cerr << "\033[31m[ERROR] Bad ADD: " << line << "\033[0m\n";
                hasInvalidInstruction = true;
                continue;
            }
            bool a1num = isDecimalNumber(a1);
            bool a2num = isDecimalNumber(a2);

            if (a1num && a2num) {
                parsedInstructions.push_back(
                    std::make_shared<AddInstruction>(target, parseU16(a1), parseU16(a2)));
            }
            else if (a1num && !a2num) {
                parsedInstructions.push_back(
                    std::make_shared<AddInstruction>(target, parseU16(a1), a2));
            }
            else if (!a1num && a2num) {
                parsedInstructions.push_back(
                    std::make_shared<AddInstruction>(target, a1, parseU16(a2)));
            }
            else {
                parsedInstructions.push_back(
                    std::make_shared<AddInstruction>(target, a1, a2));
            }
        }
        // ========== SUBTRACT ==========
        else if (keyword == "subtract") {
            std::stringstream ss(args);
            std::string target, a1, a2;
            ss >> target >> a1 >> a2;
            trim_in_place(target); trim_in_place(a1); trim_in_place(a2);
            if (target.empty() || a1.empty() || a2.empty()) {
                std::cerr << "\033[31m[ERROR] Bad SUBTRACT: " << line << "\033[0m\n";
                hasInvalidInstruction = true;
                continue;
            }
            bool a1num = isDecimalNumber(a1);
            bool a2num = isDecimalNumber(a2);

            if (a1num && a2num) {
                parsedInstructions.push_back(
                    std::make_shared<SubtractInstruction>(target, parseU16(a1), parseU16(a2)));
            }
            else if (a1num && !a2num) {
                parsedInstructions.push_back(
                    std::make_shared<SubtractInstruction>(target, parseU16(a1), a2));
            }
            else if (!a1num && a2num) {
                parsedInstructions.push_back(
                    std::make_shared<SubtractInstruction>(target, a1, parseU16(a2)));
            }
            else {
                parsedInstructions.push_back(
                    std::make_shared<SubtractInstruction>(target, a1, a2));
            }
        }
        // ========== WRITE ==========
        else if (keyword == "write") {
            std::stringstream ss(args);
            std::string addrStr, var;
            ss >> addrStr >> var;
            trim_in_place(addrStr); trim_in_place(var);
            if (addrStr.empty() || var.empty()) {
                std::cerr << "\033[31m[ERROR] Bad WRITE: " << line << "\033[0m\n";
                hasInvalidInstruction = true;
                continue;
            }
            uint16_t addr = parseAddress(addrStr);
            parsedInstructions.push_back(std::make_shared<WriteInstruction>(addr, var));
        }
        // ========== READ ==========
        else if (keyword == "read") {
            std::stringstream ss(args);
            std::string var, addrStr;
            ss >> var >> addrStr;
            trim_in_place(var); trim_in_place(addrStr);
            if (var.empty() || addrStr.empty()) {
                std::cerr << "\033[31m[ERROR] Bad READ: " << line << "\033[0m\n";
                hasInvalidInstruction = true;
                continue;
            }
            uint16_t addr = parseAddress(addrStr);
            parsedInstructions.push_back(std::make_shared<ReadInstruction>(var, addr));
        }
        // ========== PRINT ==========
        else if (keyword == "print") {
            // Reuse your existing helper; it will push the right PrintInstruction(s).
            handlePrintInstruction(line, parsedInstructions);
        }
        // ========== UNKNOWN ==========
        else {
            std::cerr << "\033[33m[WARNING] Unknown instruction: " << keyword << "\033[0m\n";
            hasInvalidInstruction = true;
        }
    }

    if (hasInvalidInstruction || parsedInstructions.empty()) {
        std::cerr << "\033[31m[ERROR] Process not created due to invalid instruction(s).\033[0m\n";
        return;
    }
    if (parsedInstructions.size() > 50) {
        std::cerr << "\033[31m[ERROR] Too many instructions (max 50).\033[0m\n";
        return;
    }

    // --- 2) Create the process and queue it ---
    auto process = std::make_shared<Process>(procName,
        parsedInstructions,
        static_cast<size_t>(procMem),
        static_cast<size_t>(memPerFrame));

    {
        std::lock_guard<std::mutex> lock(processesMutex);
        processes.push_back(process);
    }
    if (schedulerType == "rr") rrScheduler->enqueueProcess(process);
    else                       fcfsScheduler->addProcess(process);

    std::cout << "\033[32m[SUCCESS] Process " << procName
        << " created with " << parsedInstructions.size()
        << " instruction(s) and queued.\033[0m\n";
}

void Console::handlePrintInstruction(
    const std::string& line,
    std::vector<std::shared_ptr<Instruction>>& out)
{
    auto trim = [](std::string& s) {
        auto sp = [](unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
        while (!s.empty() && sp((unsigned char)s.front())) s.erase(s.begin());
        while (!s.empty() && sp((unsigned char)s.back()))  s.pop_back();
        };

    // 1) pull out the argument text between '(' and ')'
    size_t lp = line.find('(');
    size_t rp = line.rfind(')');
    if (lp == std::string::npos || rp == std::string::npos || rp <= lp) {
        std::cerr << "\033[31m[ERROR] Bad PRINT syntax: " << line << "\033[0m\n";
        return;
    }
    std::string args = line.substr(lp + 1, rp - lp - 1);

    // helper: find the next UNESCAPED quote (") in args starting at pos
    auto find_unescaped_quote = [&](size_t start) -> size_t {
        size_t pos = start;
        while (true) {
            pos = args.find('"', pos);
            if (pos == std::string::npos) return pos;
            // count preceding backslashes
            size_t b = 0, k = pos;
            while (k > 0 && args[k - 1] == '\\') { ++b; --k; }
            if ((b % 2) == 0) return pos;   // even number of '\' -> unescaped
            ++pos;                           // keep scanning
        }
        };

    // 2) find the quoted literal (supports both "..." and \"...\")
    size_t qStart = std::string::npos;
    size_t qEnd = std::string::npos;

    // first try normal, unescaped quotes (PowerShell etc.)
    size_t uq1 = find_unescaped_quote(0);
    if (uq1 != std::string::npos) {
        size_t uq2 = find_unescaped_quote(uq1 + 1);
        if (uq2 != std::string::npos) {
            qStart = uq1;            // points at the opening "
            qEnd = uq2;            // points at the closing "
        }
    }

    // fallback: cmd.exe style \"...\"
    if (qStart == std::string::npos) {
        size_t e1 = args.find("\\\"");
        if (e1 != std::string::npos) {
            size_t e2 = args.find("\\\"", e1 + 2);
            if (e2 != std::string::npos) {
                // include the backslash+quote pair on both sides
                qStart = e1;            // backslash before the opening "
                qEnd = e2 + 1;        // index of the " in the second pair
            }
        }
    }

    if (qStart == std::string::npos || qEnd == std::string::npos || qEnd <= qStart) {
        std::cerr << "\033[31m[ERROR] PRINT needs a quoted literal: " << line << "\033[0m\n";
        return;
    }

    // 3) take the literal *including* whatever quoting/escapes are present.
    //    PrintInstruction::unescapeAndStripQuotes() will clean it up.
    std::string literalToken = args.substr(qStart, qEnd - qStart + 1);

    // 4) optional "+ var" after the literal
    std::string var;
    bool hasVar = false;
    size_t plus = args.find('+', qEnd + 1);
    if (plus != std::string::npos) {
        var = args.substr(plus + 1);
        trim(var);
        hasVar = !var.empty();
    }

    if (hasVar)
        out.push_back(std::make_shared<PrintInstruction>(literalToken, var));
    else
        out.push_back(std::make_shared<PrintInstruction>(literalToken));
}
