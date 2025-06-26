#include "Console.h"


#include "Console.h"
#include <iostream>
using namespace std;

Console::Console(Scheduler& sched, int cpuCount, int timeQuantum, int batchFreq, int minIns, int maxIns, int delay, std::string schedulerType)
    : scheduler(sched), cpuCount(cpuCount), timeQuantum(timeQuantum),
    batchProcessFreq(batchFreq), minInstructions(minIns), maxInstructions(maxIns),
    delayPerExecution(delay), schedulerType(schedulerType), schedulerRunning(false)
{
    if (dynamic_cast<RRScheduler*>(&sched)) {
        rrScheduler = std::make_unique<RRScheduler>(cpuCount, timeQuantum);
    }
    else {
        fcfsScheduler = std::make_unique<FCFSScheduler>(cpuCount);
        this->timeQuantum = 0;
    }
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
    // Banner with green color
    std::cout << "\033[32m"; // Set green
    std::cout << "===============================\n";
    std::cout << "|    SYSTEM INITIALIZATION    |\n";
    std::cout << "===============================";
    std::cout << "\033[0m";  // Reset color

    // Do the real work (untouched)
    processes.clear();
    schedulerRunning = false;

    for (int i = 1; i <= 10; ++i) {
        string processName = "P" + to_string(i);
        int commands = 5 + (rand() % 100);
        size_t memory = 512 + (i * 64);
        auto process = make_shared<Process>(processName, commands, memory);
        processes.push_back(process);


        if (schedulerType == "rr") {
            rrScheduler->enqueueProcess(process);
        }
        else {
            fcfsScheduler->addProcess(process);
        }
    }

    // Confirmation summary in cyan
    std::cout << "\033[36m"; // Set cyan
    std::cout << "\nCreated " << processes.size() << " processes\n\n";
    std::cout << "\033[0m"; // Reset color
}



void Console::schedulerStart() {
    clear();

    if (processes.empty()) {
        std::cout << "\033[36m"; // Cyan
        std::cout << "No processes to schedule. Use 'initialize' first.\n";
        std::cout << "\033[0m";
        return;
    }

    if (schedulerType == "rr") {
        // Header
        std::cout << "\033[32m"; // Green
        std::cout << "=====================================\n";
        std::cout << "|      SCHEDULER START (RR)         |\n";
        std::cout << "=====================================\n";
        std::cout << "\033[0m";

        std::cout << "\033[36m"; // Cyan
        std::cout << "Starting Round Robin scheduler with " << cpuCount << " CPUs and time quantum " << timeQuantum << "\n";
        rrScheduler->start();
        schedulerRunning = true;
        std::cout << "Scheduler started successfully.\n\n";
        std::cout << "\033[0m";
    } else {
        // Header
        std::cout << "\033[32m"; // Green
        std::cout << "=====================================\n";
        std::cout << "|    SCHEDULER START (FCFS)         |\n";
        std::cout << "=====================================\n";
        std::cout << "\033[0m";

        std::cout << "\033[36m"; // Cyan
        std::cout << "Starting FCFS scheduler with " << cpuCount << " CPUs\n";
        fcfsScheduler->start();
        schedulerRunning = true;
        std::cout << "FCFS scheduler started successfully.\n\n";
        std::cout << "\033[0m";
    }
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
        for (const auto& process : processes) {
            process->displayProcess(); 
        }
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
        if (schedulerType == "rr") {
            std::cout << "Stopping Round Robin scheduler...\n";
            rrScheduler->stop();
            std::cout << "Scheduler stopped successfully.\n";
        }
        else {
            std::cout << "Stopping FCFS scheduler...\n";
            fcfsScheduler->stop();
            std::cout << "FCFS scheduler stopped successfully.\n";
        }
        schedulerRunning = false;
    }
    else {
        std::cout << "Scheduler is not currently running.\n";
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
        for (const auto& process : processes) {
            process->displayProcess(file);  // assume overloaded
        }
    }
    else {
        fcfsScheduler->displayProcesses(file);  // also overloaded
    }


    file << "=====================================================================\n";
    file.close();

    cout << "Report generated at ./Debug/csopesy-log.txt! << endl" << endl;
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
    else if (userInput.rfind("screen -s ", 0) == 0 ||
        userInput.rfind("screen -r ", 0) == 0) {
        // pull off everything after the space+flag, e.g. "P1"
        auto procName = userInput.substr(userInput.find_last_of(' ') + 1);
        showProcessScreen(procName);
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