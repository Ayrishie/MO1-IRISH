#include "Console.h"
#include <cstdlib>
#include "Console.h"
#include <iostream>
#include <filesystem>

// check for OS Platform for single MACRO definition for terminal clearing
#ifdef _WIN32
#define SYSCLEAR system("cls");
#else
#define SYSCLEAR system("clear");
#endif

using namespace std;

//Handling File of process
const std::string logDir = "processesLogs";
namespace fs = std::filesystem;

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
    cout << "  screen -r <name> - Display screen for a process" << endl;
    cout << "  screen -s <name> - Create a new screen for a process" << endl;
    cout << "  screen -ls       - List system utilization and processes" << endl;
    cout << "  scheduler-start - Start scheduler" << endl;
    cout << "  scheduler-stop - Stop scheduler" << endl;
    cout << "  report-util    - Report system utilization" << endl;
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

    if (isInitialized) {
        std::cout << "\033[33m";
        std::cout << "System was already initialized then Run. Please Restart the program.\n";
        std::cout << "\033[0m";
        return;
    }

    clear();

    std::ifstream config("config.txt");
    if (!config.is_open()) {
        std::cerr << "Error: Could not open config.txt\n";
        return;
    }


    // Set default values (matches old main.cpp)
    cpuCount = 1;
    timeQuantum = 0;
    batchProcessFreq = 1;
    minInstructions = 1000;
    maxInstructions = 2000;
    delayPerExecution = 0;
    schedulerType = "fcfs";

    // Create the directory if it does not exist
    if (!fs::exists(logDir)) {
        fs::create_directory(logDir);
    }

    // Create the directory for Snapshots
    setupSnapshotDirectory();

    // If it exists and is a directory, delete .txt files
    if (fs::is_directory(logDir)) {
        for (const auto& entry : fs::directory_iterator(logDir)) {
            if (entry.path().extension() == ".txt") {
                fs::remove(entry);
            }
        }

        std::string key, value;
        while (config >> key >> value) {
            // Remove quotes if any
            value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());

            if (key == "scheduler") schedulerType = value;
            else if (key == "num-cpu") cpuCount = std::stoi(value);
            else if (key == "quantum-cycles") timeQuantum = std::stoi(value);
            else if (key == "batch-process-freq") {
                batchProcessFreq = std::stoi(value);
                if (batchProcessFreq < 1) {
                    std::cerr << "\033[31mError: batch-process-freq must be at least 1\033[0m\n";
                    return;
                }
            }
            else if (key == "min-ins") minInstructions = std::stoi(value);
            else if (key == "max-ins") maxInstructions = std::stoi(value);
            else if (key == "delay-per-exec") delayPerExecution = std::stoi(value);
            else if (key == "max-overall-mem") maxOverallMem = std::stoi(value);
            else if (key == "mem-per-frame") memPerFrame = std::stoi(value);
            else if (key == "mem-per-proc") memPerProc = std::stoi(value);
            else std::cout << "Warning: unknown key \"" << key << "\" skipped\n";
        }

        if (!memoryCheck(maxOverallMem, memPerFrame, memPerProc)) {
            return;
        }

        // Initialize Memoery Manager
        memoryManager = std::make_unique<MemoryManager>(maxOverallMem, memPerProc);

        // Show config summary
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
        std::cout << "Mem per Proc: " << memPerProc << "bytes\n";
        std::cout << "\033[0m";

        processes.clear();
        schedulerRunning = false;

        // Scheduler init
        if (schedulerType == "rr") {
            rrScheduler = std::make_unique<RRScheduler>(cpuCount, timeQuantum, delayPerExecution, memoryManager.get());
        }
        else if (schedulerType == "fcfs") {
            fcfsScheduler = std::make_unique<FCFSScheduler>(cpuCount, delayPerExecution, memoryManager.get());
        }
        else {
            std::cerr << "Error: unknown scheduler type '" << schedulerType << "' in config.txt\n";
        }
    }
}

bool Console:: memoryCheck(int maxMem, int frameSize, int procMem) {
    auto isPowerOfTwo = [](int x) {
        return x > 0 && (x & (x - 1)) == 0;
        };

    if (!isPowerOfTwo(maxMem) || !isPowerOfTwo(frameSize) || !isPowerOfTwo(procMem)) {
        std::cerr << "\033[31mError: Memory values must be powers of 2\033[0m\n";
        return false;
    }

    if (maxMem < 64 || maxMem > 65536 ||
        frameSize < 16 || frameSize > 65536 || // Change this to 64 after
        procMem < 64 || procMem > 65536) {
        std::cerr << "\033[31mError: Memory values must be in range [64, 65536]\033[0m\n";
        return false;
    }

    if (maxMem % frameSize != 0) {
        std::cerr << "\033[31mError: maxOverallMem must be divisible by memPerFrame\033[0m\n";
        return false;
    }

    if (procMem > maxMem) {
        std::cerr << "\033[31mError: memPerProc exceeds maxOverallMem\033[0m\n";
        return false;
    }

    return true;
}



void Console::schedulerStart() {
    clear();

    if (!fcfsScheduler && !rrScheduler) {
        std::cerr << "Error: Scheduler not initialized. Please run initialize first.\n";
        return;
    }

    if (schedulerRunning) {
        std::cout << "Scheduler is already running!\n";
        return;
    }

    if (schedulerThread.joinable()) {
        std::cout << "Scheduler thread is already running!\n";
        return;
    }

    schedulerRunning = true;

    // Header depending on scheduler type
    std::cout << "\033[32m";
    std::cout << "=====================================\n";
    std::cout << "|      SCHEDULER START (" << (schedulerType == "rr" ? "RR" : "FCFS") << ")       |\n";
    std::cout << "=====================================\n";
    std::cout << "\033[0m";

    std::cout << "\033[36m";
    if (schedulerType == "rr") {
        std::cout << "Starting Round Robin scheduler with " << cpuCount << " CPUs and time quantum " << timeQuantum << "\n";
        rrScheduler->start();

    }
    else {
        std::cout << "Starting FCFS scheduler with " << cpuCount << " CPUs\n";
        fcfsScheduler->start();
    }
    std::cout << "Process will be generated every " << batchProcessFreq << " ticks\n";
    std::cout << "\033[0m";

    schedulerThread = std::thread([this]() {
        int tick = 0;
        int quantumCycle = 0;


        while (schedulerRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExecution));
            tick++;

            if (tick % batchProcessFreq == 0) {
                std::ostringstream nameStream;
                nameStream << "p" << std::setfill('0') << std::setw(2) << ++pidCounter;
                std::string name = nameStream.str();
                int commands = minInstructions + (rand() % (maxInstructions - minInstructions + 1));
                size_t memory = memPerProc;
                auto process = std::make_shared<Process>(name, commands, memory);
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


                //std::cout << "\033[36m[Tick " << tick << "] Created process: " << name
                //    << " with " << commands << " instructions\n\033[0m";
            }

            // 2) snapshot every timeQuantum ticks
            if (timeQuantum > 0 && tick % timeQuantum == 0) {
                ++quantumCycle;
                dumpMemorySnapshot(tick);
            }

            // Optional: Add per-tick scheduler logic here if needed
        }
        });

    isInitialized = true;
    std::cout << "\033[36mScheduler started successfully.\n\n\033[0m";
}

void Console::createProcessFromCommand(const std::string& procName) {
    if (procName.empty()) {
        std::cout << "Error: Process name required.\n";
        return;
    }

    if (!fcfsScheduler && !rrScheduler) {
        std::cerr << "Error: Scheduler not initialized. Please run initialize first.\n";
        return;
    }

    {
        std::lock_guard<std::mutex> lock(processesMutex);
        for (const auto& p : processes) {
            if (p->getName() == procName) {
                std::cout << "Process \"" << procName << "\" already exists.\n";
                return;
            }
        }
    }
    int commands = minInstructions + (rand() % (maxInstructions - minInstructions + 1));
    size_t memory = memPerProc;
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
        std::cout << "\033[32mCreated process \"" << procName << "\" with " << commands << " instructions.\033[0m\n";
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
        std::cerr << "Error: Process \"" << procName << "\" not found.\n";
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
        std::cout << "Process not found: " << procName << "\n";
        return;
    }

    // Clear screen MACRO
    SYSCLEAR;

    // Main loop
    while (true) {
        // A) Header
        std::cout << "Process name: " << procPtr->name << "\n";
        std::cout << "ID:           " << procPtr->process_id << "\n\n";

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
            auto & instrs = procPtr->getInstructions();
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
                std::cout << "Terminating all processes...\n";
            }
            else if (cmd == "pause") {
                paused = true;
                std::cout << "Test paused. Type 'continue' to resume.\n";
            }
            else if (cmd == "continue") {
                paused = false;
                std::cout << "Resuming test...\n";
            }
            else if (cmd == "exit-test") {
                exitTest = true;
                std::cout << "Exiting test mode...\n";
            }
            else {
                std::cout << "Invalid command in test mode. Valid commands: stop, pause, continue, exit-test\n";
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




void Console::parseInput(string userInput) {
    transform(userInput.begin(), userInput.end(), userInput.begin(), ::tolower);


    if (userInput == "exit") {
        // immediate, no-destructors termination:
        std::_Exit(EXIT_SUCCESS);
    }

    if (userInput == "initialize") {
        initialize();
    }
    else if (userInput == "screen -ls") {
        screen();
    }
    else if (userInput == "scheduler-start") {
        schedulerStart();
    }
    // NEW: attach to a process’s “screen”
    else if (userInput.rfind("screen -s ", 0) == 0) {
        auto procName = userInput.substr(userInput.find_last_of(' ') + 1);
        createProcessFromCommand(procName);
    }
    else if (userInput.rfind("screen -r ", 0) == 0) {
        auto procName = userInput.substr(userInput.find_last_of(' ') + 1);
        attachToProcessScreen(procName);
    }
    else if (userInput == "scheduler-test") {
        schedulerTest();
    }
    else if (userInput == "scheduler-stop") {
        schedulerStop();
    }
    else if (userInput == "report-util") {
        reportUtil();
    }
    else if (userInput == "clear") {
        clear();
    }
    else if (userInput != "exit") {
        cout << "Unknown command: " << userInput << "\n";
    }
}

void Console::dumpMemorySnapshot(int tick) {
    const std::string fname =
        "memory_stamps/memory_stamp_" + std::to_string(tick) + ".txt";

    // Destructor closes the file immediately
    {
        std::ofstream ofs(fname);
        if (!ofs) {
            std::cerr << "ERROR: Could not open \"" << fname << "\" for writing\n";
            return;
        }

        // 1) Timestamp
        ofs << "Timestamp: " << getCurrentTime() << "\n";

        // 2) Processes in memory
        auto blocks = memoryManager->getBlocksSnapshot();
        int inMem = std::count_if(blocks.begin(), blocks.end(),
            [](const MemoryBlock& b) { return b.occupied; });
        ofs << "Number of processes in memory: " << inMem << "\n";

        // 3) External fragmentation
        auto fragBytes = memoryManager->getExternalFragmentation();
        ofs << "Total external fragmentation in KB: " << (fragBytes / 1024) << "\n\n";

        // 4) ----end----
        ofs << "----end---- = " << memoryManager->getTotalMemory() << "\n\n";

        // 5) Memory layout high→low
        for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
            if (!it->occupied) continue;
            int lower = it->start;
            int upper = it->start + it->size;
            ofs << upper << "\n"
                << it->processName << "\n"
                << lower << "\n\n";
        }

        // 6) ----start----
        ofs << "----start---- = 0\n";
    }  
}

void Console::setupSnapshotDirectory() {

        // On Windows, raise the CRT file‐handle limit ti  open many snapshots
#ifdef _WIN32
    int newLimit = _setmaxstdio(4096);
    if (newLimit < 4096) {
        std::cerr << "Warning: _setmaxstdio only raised to "
                    << newLimit << " handles\n";
    }
#endif

    const std::string snapDir = "memory_stamps";

    // If it doesn't exist, create it; if it does, wipe old .txt files
    if (!fs::exists(snapDir)) {
        fs::create_directory(snapDir);
    }
    else if (fs::is_directory(snapDir)) {
        for (auto& entry : fs::directory_iterator(snapDir)) {
            if (entry.path().extension() == ".txt")
                fs::remove(entry);
        }
    }
    else {
        std::cerr << "Warning: \"" << snapDir
            << "\" exists and is not a directory.\n";
    }
}