#pragma once
#include <string>
#include <vector>
#include <mutex>  

struct MemoryBlock {
    int start;
    int size;
    bool occupied;
    std::string processName;
};

class MemoryManager {
private:
    int totalMemory;
    int memPerProc;
    std::vector<MemoryBlock> memoryBlocks;

    mutable std::mutex memMutex;
public:
    MemoryManager(int maxMem, int procMem);

    bool allocate(const std::string& processName);
    void free(const std::string& processName);
    void reset();
    void printMemoryLayout(); // for debug
    const std::vector<MemoryBlock>& getBlocks() const; // for snapshot/logging

    bool isAllocated(const std::string& processName) const;
};
