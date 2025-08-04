#ifndef MEMORYALLOCATOR_H
#define MEMORYALLOCATOR_H

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <queue>
#include <list>
#include <vector>
#include <string>
#include <atomic>

class Process;

//------------------------------------------------------------------------------
// MemoryAllocator (interface)
//------------------------------------------------------------------------------
class MemoryAllocator {
public:
    virtual ~MemoryAllocator() = default;

    virtual void allocate(std::shared_ptr<Process> process, std::size_t pageNumber) = 0;
    virtual void deallocate(std::shared_ptr<Process> process) = 0;
    virtual void visualizeMemory() const = 0;
};

//------------------------------------------------------------------------------
// PagingAllocator (LRU demand paging + backing store)
//------------------------------------------------------------------------------
class PagingAllocator : public MemoryAllocator {
private:
    std::size_t maxMemorySize;
    std::size_t memoryPerFrame;
    std::size_t numFrames;

    // backing store + stats
    std::string backingStoreFile = "csopesy-backing-store.txt";
    std::unordered_map<int, std::unordered_map<std::size_t, std::size_t>> processBackingStore;
    std::size_t nextBackingStoreOffset = 0;

    std::atomic<std::size_t> totalPagedIn{ 0 };
    std::atomic<std::size_t> totalPagedOut{ 0 };
    std::atomic<std::size_t> totalMemoryUsed{ 0 };

    // frames and LRU
    std::unordered_map<std::size_t, int> frameMap; // frame -> pid
    std::queue<std::size_t> freeFrameList;
    std::list<std::size_t> accessedFrames;

    // reverse mapping (frame -> owner info)
    struct OwnerInfo {
        int pid = -1;
        std::size_t pageNumber = 0;
        std::weak_ptr<Process> proc;
    };
    std::unordered_map<std::size_t, OwnerInfo> frameOwner;

    // backing-store helpers (implementation lives in .cpp)
    void savePageToBackingStore(int pid, std::size_t pageNumber, const void* data); 
    void loadPageFromBackingStore(int pid, std::size_t pageNumber, void* data);


    //alocate mute
    bool verbosePaging = false;                 // default: no console spam
    std::string pagerLogPath = "csopesy-pager.log";
    //dellocate mute
    bool verbose_ = true;
    void logPagerLine(const std::string& line) const;



public:
    PagingAllocator(std::size_t maxMemorySize, std::size_t memoryPerFrame);
    ~PagingAllocator() override = default;

    // MemoryAllocator API
    void allocate(std::shared_ptr<Process> process, std::size_t pageNumber) override;
    void deallocate(std::shared_ptr<Process> process) override;
    void visualizeMemory() const override;

    // scheduler/console helpers
    bool   isAllocated(const std::shared_ptr<Process>& process) const;
    void   evictFrame();

    // stats getters (used by console)
    std::size_t getTotalPagedIn()   const { return totalPagedIn.load(); }     // pages paged in
    std::size_t getTotalPagedOut()  const { return totalPagedOut.load(); }    // pages paged out
    std::size_t getTotalMemoryUsed() const { return totalMemoryUsed.load(); } // bytes used
    std::size_t getTotalMemory()     const { return maxMemorySize; }          // bytes managed
    std::size_t getFreeMemory()      const {
        // frames not mapped * frame size
        return (numFrames - frameMap.size()) * memoryPerFrame;
    }
    void setVerbosePaging(bool on) { verbosePaging = on; }
    bool isVerbosePaging() const { return verbosePaging; }

    void setVerbose(bool v) { verbose_ = v; }

    // OPTIONAL: quick helpers
    std::size_t getProcessResidentPages(int pid) const;
    std::size_t getProcessResidentBytes(int pid) const;
};

#endif // MEMORYALLOCATOR_H
