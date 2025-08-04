#pragma once

#include <cstddef>      // for size_t
#include <memory>       // for std::shared_ptr
#include <unordered_map>
#include <queue>
#include <list>
#include <vector>

// forward‐declare Process so we don’t need to pull in its header here:
class Process;

//------------------------------------------------------------------------------
// MEMORYALLOCATOR: INTERFACE
//------------------------------------------------------------------------------
class MemoryAllocator {
public:
    virtual ~MemoryAllocator() = default;

    // demand‐page 'pageNumber' of 'process'
    virtual void allocate(std::shared_ptr<Process> process,
        std::size_t pageNumber) = 0;

    // free all pages belonging to 'process'
    virtual void deallocate(std::shared_ptr<Process> process) = 0;

    // for "process-smi" / debugging
    virtual void visualizeMemory() const = 0;
};

//------------------------------------------------------------------------------
// PAGINGALLOCATOR: concrete implementation of MemoryAllocator
//------------------------------------------------------------------------------
class PagingAllocator : public MemoryAllocator {
private:
    std::size_t maxMemorySize;           // total bytes of physical memory
    std::size_t memoryPerFrame;          // bytes in each frame/page
    std::size_t numFrames;               // = maxMemorySize / memoryPerFrame

    // frame → process_id map
    std::unordered_map<std::size_t, int> frameMap;

    // free frames pool
    std::queue<std::size_t> freeFrameList;

    // for LRU: tracks frames in order of “most recently used” at back
    std::list<std::size_t> accessedFrames;

public:
    PagingAllocator(std::size_t maxMemorySize, std::size_t memoryPerFrame);
    ~PagingAllocator() override = default;



    

    

    // MemoryAllocator API
    void allocate(std::shared_ptr<Process> process,
        std::size_t pageNumber) override;

    // bring *all* pages of a process into memory at once
    void allocate(std::shared_ptr<Process> process);

    void deallocate(std::shared_ptr<Process> process) override;
    void visualizeMemory() const override;
// returns true if *any* of this process’s pages are resident
bool isAllocated(const std::shared_ptr<Process>&process) const;

    // Kick out the least-recently-used frame
    void evictFrame();
};
