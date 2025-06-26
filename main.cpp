// main.cpp

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <algorithm> 

#include "Console.h"
#include "FCFSScheduler.h"
#include "RRScheduler.h"

int main() {
    std::ifstream cfg("Config.txt");
    if (!cfg) {
        std::cerr << "Error: could not open config.txt\n";
        return 1;
    }
    std::string key, value, schedulerType;
    int numCpu = 1;
    int quantumCycles = 0;
    int batchProcessFreq = 1;
    int minIns = 1000;
    int maxIns = 2000;
    int delayPerExec = 0;

    while (cfg >> key >> value) {
        // Remove quotes if any
        value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());

        if (key == "scheduler") {
            schedulerType = value;
        }
        else if (key == "num-cpu") {
            numCpu = std::stoi(value);
        }
        else if (key == "quantum-cycles") {
            quantumCycles = std::stoi(value);
        }
        else if (key == "batch-process-freq") {
            batchProcessFreq = std::stoi(value);
        }
        else if (key == "min-ins") {
            minIns = std::stoi(value);
        }
        else if (key == "max-ins") {
            maxIns = std::stoi(value);
        }
        else if (key == "delay-per-exec") {
            delayPerExec = std::stoi(value);
        }
        else {
            std::cout << "Warning: unknown key \"" << key << "\" skipped\n";
        }
    }

    
    std::unique_ptr<Scheduler> sched;

    if (schedulerType == "fcfs") {
        sched = std::make_unique<FCFSScheduler>(numCpu);
    }
    else if (schedulerType == "rr") {
        sched = std::make_unique<RRScheduler>(numCpu, quantumCycles);
    }
    else {
        std::cerr << "Error: unknown scheduler type '" << schedulerType << "' in config.txt\n";
        return 1;
    }



    Console console(*sched, numCpu, quantumCycles, batchProcessFreq, minIns, maxIns, delayPerExec, schedulerType);
    
    console.header();
    console.start();

    return 0;

}
