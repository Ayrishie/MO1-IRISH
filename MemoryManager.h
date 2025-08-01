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
    ~MemoryManager() = default;

    bool allocate(const std::string& processName);
    void free(const std::string& processName);
    void reset();
    bool isAllocated(const std::string& processName) const;

    std::vector<MemoryBlock> getBlocksSnapshot() const;
    int getExternalFragmentation() const;
    const std::vector<MemoryBlock>& getBlocks() const;
    int getTotalMemory() const;

};
