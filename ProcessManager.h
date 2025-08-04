#pragma once
#include <memory>
#include <unordered_map>
#include <mutex>

class Process;  // Forward declaration

class ProcessManager {
private:
    std::unordered_map<int, std::weak_ptr<Process>> processMap;
    std::mutex mapMutex;

public:
    void addProcess(std::shared_ptr<Process> process);
    std::shared_ptr<Process> getProcess(int pid);
    void removeProcess(int pid);
};