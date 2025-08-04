#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <cstddef>

// Holds all configuration parameters loaded from Config.txt
struct Config {
    int numCpu;                  // Number of virtual CPUs to simulate
    std::string scheduler;       // "fcfs" or "rr"
    int quantumCycles;           // Time-slice length for RR scheduling
    int batchProcessFreq;        // Frequency of spawning batch processes
    int minIns;                  // Minimum instructions per process
    int maxIns;                  // Maximum instructions per process
    int delayPerExec;            // Delay (in ticks) per instruction execution
    std::size_t maxOverallMem;   // Total memory pool size in bytes
    std::size_t memPerFrame;     // Size of each memory frame in bytes
    std::size_t minMemPerProc;   // Minimum memory allocation per process
    std::size_t maxMemPerProc;   // Maximum memory allocation per process
};

// Parse the configuration file at 'filepath' and return a Config struct.
// Throws std::runtime_error on parse errors or invalid values.
Config loadConfig(const std::string& filepath);

#endif // CONFIG_H
