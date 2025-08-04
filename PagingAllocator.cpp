#include "MemoryAllocator.h"
#include "Process.h"
#include "ProcessManager.h"

#include <iostream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <mutex>  
//
// PagingAllocator ctor: carve physical memory into fixed‐size frames
//
PagingAllocator::PagingAllocator(
    size_t maxMemorySize,
    size_t memoryPerFrame,
    ProcessManager& pm)
    : maxMemorySize(maxMemorySize),
    memoryPerFrame(memoryPerFrame),
    numFrames((maxMemorySize + memoryPerFrame - 1) / memoryPerFrame),
    processManager(pm)  // Initialize reference
    {

    backingStoreFile = "csopesy-backing-store.txt";

    // Clear any existing backing store
    std::ofstream clearFile(backingStoreFile, std::ios::trunc);
    clearFile.close();

    // Initialize free frames
    for (size_t i = 0; i < numFrames; ++i) {
        freeFrameList.push(i);
    }
    std::cout << "[DEBUG] Backing store initialized at " << backingStoreFile << "\n";

    // backing store logs
    backingStoreFile = R"(C:\Users\Ernest Balderosa\Desktop\MO1-IRISH\csopesy-backing-store.txt)";

    logEvent("Backing store location: " + backingStoreFile);

    backingStoreLog.open("backing_store_operations.log", std::ios::trunc);
    logEvent("=== Backing Store Initialized ===");
    logEvent("Config: " + std::to_string(numFrames) + " frames, " +
        std::to_string(memoryPerFrame) + " bytes/frame");

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
    if (accessedFrames.empty()) {
        logEvent("Eviction failed: no frames to evict");
        return;
    }

    size_t frame = accessedFrames.front();
    auto it = frameMap.find(frame);

    if (it == frameMap.end()) {
        logEvent("ERROR: Frame " + std::to_string(frame) + " not allocated");
        return;
    }

    int pid = it->second;
    logEvent("Evicting frame " + std::to_string(frame) + " from PID " + std::to_string(pid));

    // Find which page was in this frame
    // Note: You'll need a way to get the process pointer here
    // This is a weak point in your current design
    std::shared_ptr<Process> proc = processManager.getProcess(pid);

    if (proc) {
        for (auto& [pnum, page] : proc->page_table) {
            if (page.frame_number == static_cast<int>(frame) && page.inMemory) {
                // Get page data before eviction
                std::vector<uint16_t> pageData = proc->getPageData(pnum);

                // Write to backing store
                writeToBackingStore(pid, pnum, pageData);
                logEvent("Page " + std::to_string(pnum) + " written to backing store");

                // Update page table
                page.inMemory = false;
                page.swapped = true;
                page.frame_number = -1;
                break;
            }
        }
    }

    // Free the frame
    frameMap.erase(frame);
    freeFrameList.push(frame);
    accessedFrames.pop_front();
}


//
// for backing store
//

void PagingAllocator::writeToBackingStore(int process_id, size_t page_number,
    const std::vector<uint16_t>& data) {
    std::lock_guard<std::mutex> lock(backingStoreMutex);

    std::ofstream out(backingStoreFile, std::ios::binary | std::ios::app);
    if (!out) {
        logEvent("ERROR: Failed to open backing store file for writing");
        return;
    }

    // Write process_id and page_number as header
    out.write(reinterpret_cast<const char*>(&process_id), sizeof(process_id));
    out.write(reinterpret_cast<const char*>(&page_number), sizeof(page_number));

    // Write data
    out.write(reinterpret_cast<const char*>(data.data()),
        data.size() * sizeof(uint16_t));

    logEvent("Wrote page " + std::to_string(page_number) +
        " for PID " + std::to_string(process_id) +
        " (" + std::to_string(data.size()) + " words)");
}

std::vector<uint16_t> PagingAllocator::readFromBackingStore(int process_id, size_t page_number){
    std::lock_guard<std::mutex> lock(backingStoreMutex);
    std::ifstream in(backingStoreFile, std::ios::binary);
    std::vector<uint16_t> data;

    if (!in) {
        logEvent("ERROR: Failed to open backing store file for reading");
        return data;
    };

    int current_pid;
    size_t current_page;

    while (in.read(reinterpret_cast<char*>(&current_pid), sizeof(current_pid))) {
        in.read(reinterpret_cast<char*>(&current_page), sizeof(current_page));

        if (current_pid == process_id && current_page == page_number) {
            // Calculate remaining bytes for this page
            auto start_pos = in.tellg();
            in.seekg(0, std::ios::end);
            auto end_pos = in.tellg();
            in.seekg(start_pos);

            size_t data_size = (end_pos - start_pos) / sizeof(uint16_t);
            data.resize(data_size);

            in.read(reinterpret_cast<char*>(data.data()),
                data_size * sizeof(uint16_t));
            break;
        }
        else {
            // Skip this page's data
            in.seekg(memoryPerFrame * sizeof(uint16_t), std::ios::cur);
        };
    }

    return data;
};



void PagingAllocator::logEvent(const std::string& message){
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
    localtime_s(&tm_buf, &in_time_t);  // Windows-safe version

    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "[%Y-%m-%d %H:%M:%S] ", &tm_buf);

    backingStoreLog << time_str << message << std::endl;
    std::cout << "[BACKING STORE] " << message << std::endl;
}

void PagingAllocator::logFrameState() {
    std::stringstream ss;
    ss << "Frame State:\n";
    for (size_t i = 0; i < numFrames; ++i) {
        ss << "Frame " << i << ": ";
        auto it = frameMap.find(i);
        ss << (it != frameMap.end() ? "PID " + std::to_string(it->second) : "FREE");
        ss << "\n";
    }
    logEvent(ss.str());
}

void PagingAllocator::updateLruPosition(size_t frame) {
    auto it = std::find(accessedFrames.begin(), accessedFrames.end(), frame);
    if (it != accessedFrames.end()) {
        accessedFrames.erase(it);
    }
    accessedFrames.push_back(frame);
}
