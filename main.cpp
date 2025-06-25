// main.cpp

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "Console.h"
#include "FCFSScheduler.h"
#include "RRScheduler.h"

int main() {
    std::ifstream cfg("Config.txt");
    if (!cfg) {
        std::cerr << "Error: could not open config.txt\n";
        return 1;
    }
    std::string key, schedulerType;
    int numCpu = 1;
    int quantumCycles = 0;

    while (cfg >> key) {
        if (key == "scheduler") {
            cfg >> schedulerType;            // “fcfs” or “rr”
        }
        else if (key == "num-cpu") {
            cfg >> numCpu;                   // e.g. 4
        }
        else if (key == "quantum-cycles") {
            cfg >> quantumCycles;            // e.g. 10 (only used for RR)
        }
        else {
            // skip unknown key’s value
            std::string dummy;
            cfg >> dummy;
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
        std::cerr << "Error: unknown scheduler type '" 
                  << schedulerType << "' in config.txt\n";
        return 1;
    }

 
    Console console(*sched, numCpu, quantumCycles);

    
    console.header();
    console.start();

    return 0;
}
