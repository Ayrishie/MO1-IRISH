#include "MemoryAllocator.h"
#include "Process.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <chrono>
#include <fstream>

// ctor: also truncate pager log once at startup
PagingAllocator::PagingAllocator(std::size_t maxMemorySize_, std::size_t memoryPerFrame_)
    : maxMemorySize(maxMemorySize_),
    memoryPerFrame(memoryPerFrame_),
    numFrames((maxMemorySize + memoryPerFrame - 1) / memoryPerFrame) {
    for (std::size_t i = 0; i < numFrames; ++i) freeFrameList.push(i);

    std::ofstream bs(backingStoreFile, std::ios::binary | std::ios::trunc);
    std::ofstream pagerLog(pagerLogPath, std::ios::trunc);   // NEW: clear pager log
}




std::size_t PagingAllocator::getProcessResidentBytes(int pid) const {
    return getProcessResidentPages(pid) * memoryPerFrame;
}

// REMOVE these two — they’re already defined inline in MemoryAllocator.h
// std::size_t PagingAllocator::getTotalMemoryUsed() const { ... }
// std::size_t PagingAllocator::getFreeMemory() const { ... }

void PagingAllocator::allocate(std::shared_ptr<Process> process, std::size_t pageNumber) {
    auto& page = process->page_table[pageNumber];
    page.page_number = pageNumber;

    if (page.inMemory && page.frame_number >= 0) {
        accessedFrames.remove(static_cast<std::size_t>(page.frame_number));
        accessedFrames.push_back(static_cast<std::size_t>(page.frame_number));
        return;
    }

    // miss -> page fault
    logPagerLine("\033[33m[PAGE FAULT] pid=" + std::to_string(process->process_id) +
        " page=" + std::to_string(pageNumber) + "\033[0m");

    if (freeFrameList.empty()) {
        evictFrame();
    }
    std::size_t frame = freeFrameList.front();
    freeFrameList.pop();

    if (page.swapped) {
        // Load actual page data from backing store
        std::vector<char> pageData(memoryPerFrame, 0);
        loadPageFromBackingStore(process->process_id, pageNumber, pageData.data());

        // In a real system, you would copy pageData to the actual frame memory
        // For simulation, we just log that we loaded it

        logPagerLine("[PAGE IN] pid=" + std::to_string(process->process_id) +
            " page=" + std::to_string(pageNumber) +
            " -> frame " + std::to_string(frame) +
            " (loaded " + std::to_string(memoryPerFrame) + " bytes from backing store)");
    }

    frameMap[frame] = process->process_id;
    accessedFrames.push_back(frame);
    frameOwner[frame] = OwnerInfo{ process->process_id, pageNumber, process };

    page.frame_number = static_cast<int>(frame);
    page.inMemory = true;
    page.swapped = false;

    totalPagedIn++;
    totalMemoryUsed = frameMap.size() * memoryPerFrame;
}





void PagingAllocator::deallocate(std::shared_ptr<Process> process) {
    int pid = process->process_id;

    // collect frames of this pid
    std::vector<std::size_t> toFree;
    for (auto const& kv : frameMap) {
        if (kv.second == pid) toFree.push_back(kv.first);
    }

    // free frames
    for (std::size_t f : toFree) {
        frameMap.erase(f);
        accessedFrames.remove(f);
        frameOwner.erase(f);
        freeFrameList.push(f);
    }

    // clear backing-store index for this pid
    processBackingStore.erase(pid);

    // mark process pages as not-in-memory
    for (auto& kv : process->page_table) {
        kv.second.frame_number = -1;
        kv.second.inMemory = false;
        // leave kv.second.swapped as-is (optional)
    }

    totalMemoryUsed = frameMap.size() * memoryPerFrame;

    // --- DEALLOCATE ---
    logPagerLine("[DEALLOCATE] Freed all pages for pid=" + std::to_string(pid));
}


// helper
static std::string nowStamp() {
    using namespace std::chrono;
    std::time_t t = system_clock::to_time_t(system_clock::now());
    std::tm tm{}; localtime_s(&tm, &t);
    std::ostringstream ss; ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void PagingAllocator::logPagerLine(const std::string& line) const {
    std::ofstream out("csopesy-pager.log", std::ios::app);
    if (out) out << "[" << nowStamp() << "] " << line << "\n";
    if (verbose_) std::cout << line << "\n";  // set verbose_=false to silence console
}






std::size_t PagingAllocator::getProcessResidentPages(int pid) const {
    std::size_t n = 0;
    for (const auto& kv : frameMap) {
        if (kv.second == pid) ++n;
    }
    return n;
}


// --- backing store helpers -------------------------------------------------
#include <fstream>
#include <filesystem>

void PagingAllocator::savePageToBackingStore(int pid,
    std::size_t pageNumber,
    const void* data) {
    // 1) Always open in append mode (will create if missing)
    std::ofstream bs(backingStoreFile,
        std::ios::binary | std::ios::app);
    if (!bs.is_open()) {
        std::cerr << "[ERROR] Cannot open backing‐store file '"
            << backingStoreFile << "'\n";
        return;
    }

    // 2) (Optional) write a small header so you can see it in a hex viewer:
    //    e.g. <PID><PAGE><SIZE> as binary or ASCII.
    //    But you can also skip this and just dump raw page bytes.
    bs.write(reinterpret_cast<const char*>(&pid), sizeof(pid));
    bs.write(reinterpret_cast<const char*>(&pageNumber), sizeof(pageNumber));
    // Now dump the actual page contents:
    bs.write(reinterpret_cast<const char*>(data), memoryPerFrame);
    bs.close();

    // 3) Log metadata for human inspection
    std::ofstream log(backingStoreFile + ".log", std::ios::app);
    log << "[WRITE] pid=" << pid
        << " page=" << pageNumber
        << " size=" << memoryPerFrame << "B\n";
}


void PagingAllocator::loadPageFromBackingStore(int pid, std::size_t pageNumber, void* out) {
    auto pidIt = processBackingStore.find(pid);
    if (pidIt == processBackingStore.end()) {
        // Page was never saved, initialize with zeros
        std::memset(out, 0, memoryPerFrame);
        return;
    }

    auto pageIt = pidIt->second.find(pageNumber);
    if (pageIt == pidIt->second.end()) {
        // Page was never saved, initialize with zeros
        std::memset(out, 0, memoryPerFrame);
        return;
    }

    std::size_t offset = pageIt->second;

    // Read actual page data from backing store
    std::ifstream bs(backingStoreFile, std::ios::binary);
    if (bs.is_open()) {
        bs.seekg(offset);
        bs.read(static_cast<char*>(out), memoryPerFrame);
        bs.close();

        // Log for debugging
        std::ofstream log(backingStoreFile + ".log", std::ios::app);
        if (log) {
            log << "[READ ] pid=" << pid
                << " page=" << pageNumber
                << " offset=" << offset
                << " status=HIT\n";
        }
    }
    else {
        // File doesn't exist, initialize with zeros
        std::memset(out, 0, memoryPerFrame);
    }
}

// Also update evictFrame() to actually save page data:
void PagingAllocator::evictFrame() {
   
    if (accessedFrames.empty()) return;


    std::size_t frame = accessedFrames.front();
    accessedFrames.pop_front();

    std::cerr << "[DEBUG] evictFrame() called, frame=" << frame << "\n";

    // identify owner
    int pid = -1;
    std::size_t pageNumber = 0;
    std::shared_ptr<Process> proc;

    auto ownIt = frameOwner.find(frame);
    if (ownIt != frameOwner.end()) {
        pid = ownIt->second.pid;
        pageNumber = ownIt->second.pageNumber;
        proc = ownIt->second.proc.lock();
        frameOwner.erase(ownIt);
    }

    // IMPORTANT: Create simulated page data to save
    // In a real system, this would be the actual frame contents
    std::vector<char> pageData(memoryPerFrame, 0);

    // Simulate some data (you could make this more realistic)
    // For example, put the pid and page number in the first few bytes
    if (memoryPerFrame >= sizeof(int) * 2) {
        *reinterpret_cast<int*>(pageData.data()) = pid;
        *reinterpret_cast<int*>(pageData.data() + sizeof(int)) = static_cast<int>(pageNumber);
    }

    // Save the actual page data
    savePageToBackingStore(pid, pageNumber, pageData.data());

    // update victim process page table
    if (proc) {
        auto it = proc->page_table.find(pageNumber);
        if (it != proc->page_table.end()) {
            it->second.frame_number = -1;
            it->second.inMemory = false;
            it->second.swapped = true;
        }
    }

    // Log page out
    logPagerLine("[PAGE OUT] evicted frame " + std::to_string(frame) +
        " (pid=" + std::to_string(pid) +
        ", page=" + std::to_string(pageNumber) + ")");

    frameMap.erase(frame);
    freeFrameList.push(frame);

    totalPagedOut++;
    totalMemoryUsed = frameMap.size() * memoryPerFrame;
}


// --- visualization / query helpers ----------------------------------------
void PagingAllocator::visualizeMemory() const {
    std::cout << "\n=== MEMORY VISUALIZATION ===\n";
    std::cout << "Total Memory: " << maxMemorySize << " bytes\n";
    std::cout << "Frame Size  : " << memoryPerFrame << " bytes\n";
    std::cout << "Total Frames: " << numFrames << "\n";
    std::cout << "Used Frames : " << frameMap.size() << "\n";
    std::cout << "Free Frames : " << (numFrames - frameMap.size()) << "\n\n";

    for (std::size_t i = 0; i < numFrames; ++i) {
        auto it = frameMap.find(i);
        if (it == frameMap.end()) {
            std::cout << "Frame " << i << " -> FREE\n";
        }
        else {
            std::cout << "Frame " << i << " -> pid " << it->second << "\n";
        }
    }
    std::cout << "================================\n\n";
}

bool PagingAllocator::isAllocated(const std::shared_ptr<Process>& process) const {
    // True if any frame belongs to this pid
    if (!process) return false;
    const int pid = process->process_id;
    for (const auto& kv : frameMap) {
        if (kv.second == pid) return true;
    }
    return false;
}
