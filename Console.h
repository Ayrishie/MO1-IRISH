#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

#include "Process.h"
#include "RRScheduler.h"
#include "FCFSScheduler.h"
#include "MemoryAllocator.h"
#include "ProcessManager.h"

class Console {
private:
    // Internal state
    std::string userInput;
    std::vector<std::shared_ptr<Process>> processes;
    std::string schedulerType;
    std::unique_ptr<RRScheduler> rrScheduler;
    std::unique_ptr<FCFSScheduler> fcfsScheduler;
    std::unique_ptr<PagingAllocator> pagingAllocator;

    bool isInitialized = false;
    std::mutex processesMutex;
    std::thread schedulerThread;

    std::atomic<bool> schedulerRunning = false;
    std::atomic<bool> testModeRunning = false;  // only once
    std::atomic<int>  pidCounter{ 0 };

    // Config & runtime parameters
    int cpuCount = 0;
    int timeQuantum = 0;
    int batchProcessFreq = 0;
    int minInstructions = 0;
    int maxInstructions = 0;
    int delayPerExecution = 0;
    int maxOverallMem = 0;
    int memPerFrame = 0;
    int minMemPerProc = 0;
    int maxMemPerProc = 0;

    // Internal helpers
    void displayContinuousUpdates();
    void showProcessScreen(const std::string& procName);
    void printUtilization(std::ostream* out) const;
    void listProcesses();

    bool isValidProcessMemory(int procMem);
    bool validateProcessCreation(const std::string& procName, int procMem);
    void handlePrintInstruction(const std::string& line,
        std::vector<std::shared_ptr<Instruction>>& parsedInstructions);
    int  generateProcessSize();

    // backingStore
    ProcessManager processManager;

public:
    Console();
    void start();
    void header();
    void menu();
    void initialize();
    void clear();
    void screen();
    void schedulerStart();
    void schedulerStop();
    void schedulerTest();
    void reportUtil();
    void createProcessFromCommand(const std::string& procName, int procMem);
    void createProcessWithInstructions(const std::string& procName,
        int procMem,
        const std::string& instructionStr);
    void attachToProcessScreen(const std::string& procName);
    void parseInput(std::string userInput);
    bool memoryCheck(int maxMem, int frameSize,
        int minMemPerProc, int maxMemPerProc);
};

#endif // CONSOLE_H
