#include "Console.h"


#include "Console.h"
#include <iostream>
#include <filesystem> 
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
            else if (key == "batch-process-freq") batchProcessFreq = std::stoi(value);
            else if (key == "min-ins") minInstructions = std::stoi(value);
            else if (key == "max-ins") maxInstructions = std::stoi(value);
            else if (key == "delay-per-exec") delayPerExecution = std::stoi(value);
            else std::cout << "Warning: unknown key \"" << key << "\" skipped\n";
        }

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
        std::cout << "\033[0m";

        processes.clear();
        schedulerRunning = false;

        // Scheduler init
        if (schedulerType == "rr") {
            rrScheduler = std::make_unique<RRScheduler>(cpuCount, timeQuantum, delayPerExecution);
        }
        else if (schedulerType == "fcfs") {
            fcfsScheduler = std::make_unique<FCFSScheduler>(cpuCount, delayPerExecution);
        }
        else {
            std::cerr << "Error: unknown scheduler type '" << schedulerType << "' in config.txt\n";
        }
    }

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

        while (schedulerRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExecution));
            tick++;

            if (tick % batchProcessFreq == 0) {
                std::ostringstream nameStream;
                nameStream << "p" << std::setfill('0') << std::setw(2) << ++pidCounter;
                std::string name = nameStream.str();
                int commands = minInstructions + (rand() % (maxInstructions - minInstructions + 1));
                size_t memory = 512 + (pidCounter * 64);
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

            // Optional: Add per-tick scheduler logic here if needed
        }
        });


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
    size_t memory = 512 + (pidCounter * 64);
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

    // Clear screen
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

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
#ifdef _WIN32
                system("cls");
#else
                system("clear");
#endif          
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
        std::cout << "No processes to schedule. Use 'initialize' first.\n";
        return;
    }

    std::cout << (schedulerType == "rr"
        ? "Starting Round Robin scheduler test with " + std::to_string(cpuCount) + " CPUs and time quantum " + std::to_string(timeQuantum)
        : "Starting FCFS scheduler test with " + std::to_string(cpuCount) + " CPUs") << "\n";

    std::cout << "Press any key to stop the test...\n";

    schedulerRunning = true;

    std::thread schedulerThread([this]() {
        if (schedulerType == "rr") {
            rrScheduler->start();
        }
        else {
            fcfsScheduler->start();
        }
        });

    std::thread displayThread(&Console::displayContinuousUpdates, this);

    std::cin.ignore(); // Clear any existing input
    std::cin.get();    // Wait for any key press

    schedulerRunning = false;

    if (schedulerType == "rr") {
        rrScheduler->stop();
    }
    else {
        fcfsScheduler->stop();
    }

    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }

    if (displayThread.joinable()) {
        displayThread.join();
    }

    std::cout << (schedulerType == "rr" ? "Scheduler test stopped.\n" : "FCFS scheduler test stopped.\n");
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