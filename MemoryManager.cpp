#include "MemoryManager.h"
#include <iostream>

MemoryManager::MemoryManager(int maxMem, int procMem)
    : totalMemory(maxMem), memPerProc(procMem) {
    memoryBlocks.push_back({ 0, totalMemory, false, "" });
}

bool MemoryManager::allocate(const std::string& processName) {
    for (auto it = memoryBlocks.begin(); it != memoryBlocks.end(); ++it) {

        // Check if the current block is free and big enough for the process
        if (!it->occupied && it->size >= memPerProc) {
            // Calculate leftover space  after allocation
            int remaining = it->size - memPerProc;



            // Update block used by the process
            it->occupied = true;
            it->size = memPerProc;
            it->processName = processName;

            // SPlit leftover space to new free block
            if (remaining > 0) {
                memoryBlocks.insert(it + 1, {
                    it->start + memPerProc, remaining, false, ""
                    });
            }

            return true; // Allocation successful
        }
    }
    return false; // Not enough memoery
}

void MemoryManager::free(const std::string& processName) {
    for (auto& block : memoryBlocks) {
        // Free the block
        if (block.occupied && block.processName == processName) {
            block.occupied = false;
            block.processName = "";
        }
    }

    // Merge adjacent free blocks
    for (size_t i = 0; i + 1 < memoryBlocks.size(); ) {
        if (!memoryBlocks[i].occupied && !memoryBlocks[i + 1].occupied) { // If both current and next blocks are free then combine.
            memoryBlocks[i].size += memoryBlocks[i + 1].size;
            memoryBlocks.erase(memoryBlocks.begin() + i + 1);
        }
        else {
            ++i; // Remove the second block 
        }
    }
}


void MemoryManager::reset() {
    memoryBlocks.clear();
    memoryBlocks.push_back({ 0, totalMemory, false, "" });
}

const std::vector<MemoryBlock>& MemoryManager::getBlocks() const {
    return memoryBlocks;
}

void MemoryManager::printMemoryLayout() {
    std::cout << "==== Memory Layout ====\n";
    for (const auto& block : memoryBlocks) {
        std::cout << "[" << block.start << " - " << (block.start + block.size - 1) << "] ";
        std::cout << (block.occupied ? block.processName : "(free)") << "\n";
    }
}


bool MemoryManager::isAllocated(const std::string& processName) const {
    std::lock_guard<std::mutex> guard(memMutex);
    for (const auto& b : memoryBlocks) {
        if (b.occupied && b.processName == processName) {
            return true;
        }
    }
    return false;
}