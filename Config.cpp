// Config.cpp
#include "Config.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

Config loadConfig(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in) throw std::runtime_error("Cannot open config file");

    Config cfg;
    std::string key, value;
    while (in >> key >> value) {
        // strip quotes
        value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
        if (key == "num-cpu")          cfg.numCpu = std::stoi(value);
        else if (key == "scheduler")        cfg.scheduler = value;
        else if (key == "quantum-cycles")   cfg.quantumCycles = std::stoi(value);
        else if (key == "batch-process-freq") cfg.batchProcessFreq = std::stoi(value);
        else if (key == "min-ins")          cfg.minIns = std::stoi(value);
        else if (key == "max-ins")          cfg.maxIns = std::stoi(value);
        else if (key == "delay-per-exec")   cfg.delayPerExec = std::stoi(value);
        else if (key == "max-overall-mem")  cfg.maxOverallMem = std::stoull(value);
        else if (key == "mem-per-frame")    cfg.memPerFrame = std::stoull(value);
        else if (key == "min-mem-per-proc") cfg.minMemPerProc = std::stoull(value);
        else if (key == "max-mem-per-proc") cfg.maxMemPerProc = std::stoull(value);
        else /* skip unknown */;
    }

    // You can add basic validation here (powers-of-two, ranges...)
    return cfg;
}
