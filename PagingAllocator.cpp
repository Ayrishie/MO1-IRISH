#include "MemoryAllocator.h"
#include "Process.h"

#include <iostream>
#include <vector>

//
// PagingAllocator ctor: carve physical memory into fixed‐size frames
//
PagingAllocator::PagingAllocator(size_t maxMemorySize, size_t memoryPerFrame)
    : maxMemorySize(maxMemorySize)
    , memoryPerFrame(memoryPerFrame)
    , numFrames((maxMemorySize + memoryPerFrame - 1) / memoryPerFrame)
{
    // all frames start out free
    for (size_t i = 0; i < numFrames; ++i) {
        freeFrameList.push(i);
    }
}

//
// Print a map of each frame → process_id or “Free”
//
void PagingAllocator::visualizeMemory() const {
    for (size_t i = 0; i < numFrames; ++i) {
        auto it = frameMap.find(i);
        if (it != frameMap.end()) {
            std::cout << "Frame " << i << " -> Process " << it->second << "\n";
        }
        else {
            std::cout << "Frame " << i << " -> Free\n";
        }
    }
    std::cout << "---------------------------\n";
}

//
// Allocate *one* page of a process on‐demand (evict LRU if full)
//
void PagingAllocator::allocate(std::shared_ptr<Process> process, size_t pageNumber) {
    auto& page = process->page_table[pageNumber];
    if (!page.inMemory) {
        if (freeFrameList.empty()) {
            evictFrame();
        }
        size_t frame = freeFrameList.front();
        freeFrameList.pop();
        frameMap[frame] = process->process_id;
        accessedFrames.push_back(frame);
        page.frame_number = static_cast<int>(frame);
        page.inMemory = true;
        page.swapped = false;
    }
}

//
// Allocate *all* pages of a process at once (helper overload)
//
void PagingAllocator::allocate(std::shared_ptr<Process> process) {
    for (auto const& entry : process->page_table) {
        allocate(process, entry.first);
    }
}

//
// Query whether *any* page of this process is resident
//
bool PagingAllocator::isAllocated(const std::shared_ptr<Process>& process) const {
    for (auto const& kv : frameMap) {
        if (kv.second == process->process_id) return true;
    }
    return false;
}

//
// Free every frame belonging to a single process
//
void PagingAllocator::deallocate(std::shared_ptr<Process> process) {
    int pid = process->process_id;
    std::vector<size_t> toFree;

    // collect then remove so we don’t invalidate the map while iterating
    for (auto const& kv : frameMap) {
        if (kv.second == pid) {
            toFree.push_back(kv.first);
        }
    }
    for (size_t f : toFree) {
        frameMap.erase(f);
        freeFrameList.push(f);
    }

    process->page_table.clear();
}

//
// Evict the least‐recently‐used frame back to the free list
//
void PagingAllocator::evictFrame() {
    if (accessedFrames.empty()) return;
    size_t frame = accessedFrames.front();
    accessedFrames.pop_front();
    frameMap.erase(frame);
    freeFrameList.push(frame);
}
