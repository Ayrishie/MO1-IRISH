#ifndef CONSOLE_H
#define CONSOLE_H

#include <iostream>
#include <algorithm>
#include <iomanip>
#include <stdlib.h>
#include <memory>
#include <chrono>
#include <ctime>
#include <thread>
#include "Process.h"
#include "RRScheduler.h"
#include "FCFSScheduler.h"
#include "Config.h"

//new
#include "Scheduler.h"
using namespace std;

class Console {
private:

    string userInput;
    unique_ptr<RRScheduler> rrScheduler;
  
    int cpuCount;
    int timeQuantum;
    atomic<bool> schedulerRunning;  
    atomic<bool> testModeRunning;
    void displayContinuousUpdates();
    unique_ptr<FCFSScheduler> fcfsScheduler;
    
    std::vector<std::shared_ptr<Process>> processes;

    // this is the change
    Scheduler& scheduler;

    int batchProcessFreq;
    int minInstructions;
    int maxInstructions;
    int delayPerExecution;
    std::string schedulerType;
       
    

public:
    Console(Scheduler& sched, int cpuCount, int timeQuantum, int batchFreq, int minIns, int maxIns, int delay, std::string schedulerType); // default constructor (non-parameterized)

    void header();
    void menu();
    void start();
    void initialize();
    void screen();
    void schedulerTest();
    void schedulerStop();
    void reportUtil();
    void clear();
    void parseInput(string userInput);
    void listProcesses();
    void schedulerStart();


    // if out == nullptr
    void printUtilization(std::ostream* out = nullptr) const;
    void showProcessScreen(const std::string& procName);
};



#endif