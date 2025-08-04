// ProcessManager.cpp
#include "Process.h"
#include "ProcessManager.h"

void ProcessManager::addProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(mapMutex);
    processMap[process->process_id] = process;
}

std::shared_ptr<Process> ProcessManager::getProcess(int pid) {
    std::lock_guard<std::mutex> lock(mapMutex);
    auto it = processMap.find(pid);
    if (it != processMap.end()) {
        return it->second.lock();
    }
    return nullptr;
}

void ProcessManager::removeProcess(int pid) {
    std::lock_guard<std::mutex> lock(mapMutex);
    processMap.erase(pid);
}