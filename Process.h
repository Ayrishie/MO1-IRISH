#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <atomic>
#include <mutex>
#include <memory>
#include <vector>
#include <ostream>
#include <unordered_map>
#include "Instruction.h"

// ---------- Paging data ----------
struct Page {
    size_t page_number;   // logical page index
    int    frame_number;  // physical frame id; -1 if not mapped
    bool   inMemory;      // resident in RAM?
    bool   swapped;       // swapped out?
};

class PagingAllocator; // forward declaration

class Process : public std::enable_shared_from_this<Process> {
    friend class PagingAllocator;  // allow PagingAllocator to access paging internals

private:
    // ---- static/logging ----
    static std::atomic<int> next_process_id;
    static std::mutex       id_mutex;
    static std::mutex       log_mutex;
    std::unique_ptr<std::ofstream> log_file;

    // ---- program state ----
    std::vector<std::shared_ptr<Instruction>> instructions;
    std::unique_ptr<ProcessContext> context;
    int  current_instruction = 0;
    bool debug = true;

    // ---- paging state (single source of truth) ----
    size_t pageSize = 0;                                // bytes per page (frame size)
    size_t numPages = 0;                                // total pages for this proc
    std::unordered_map<size_t, Page> page_table;        // page_number -> Page

    // map a 16-bit address to (page, offset)
    std::pair<size_t, size_t> getPageAndOffset(uint16_t addr) const;
    void ensureResident(uint16_t address);

    // for backing store
    PagingAllocator* pagingAllocator;


public:
    // ---- public identity/state ----
    std::string name;
    int total_commands = 0;
    std::atomic<int> executed_commands{ 0 };
    std::chrono::time_point<std::chrono::system_clock> start_time;
    std::atomic<int> core_id{ -1 };
    int    process_id = -1;
    size_t processMemory = 0;   // bytes

    // ---- ctors/dtor ----
    Process(const std::string& pname, int commands, size_t memory);
    Process(const std::string& pname,
        const std::vector<std::shared_ptr<Instruction>>& instrs,
        size_t processMemory,
        size_t memoryPerFrame);
    ~Process();

    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    // ---- API ----
    std::string getFormattedTime() const;
    std::string getStatus() const;
    std::string getName() const;
    void        displayProcess() const;
    void        displayProcessInfo() const;
    bool        isFinished() const;
    std::string getCoreAssignment() const;

    void displayProcess(std::ostream& out) const; 
       
    bool        executeCommand(int coreId);

    void setInstructions(const std::vector<std::shared_ptr<Instruction>>& instrs);
    const std::vector<std::shared_ptr<Instruction>>& getInstructions() const { return instructions; }

    int  getCurrentInstructionLine() const { return current_instruction; }
    std::shared_ptr<Instruction> getNextInstruction() const { return instructions[current_instruction]; }

    // rolling buffer for colored PRINT lines
    static constexpr size_t MAX_BUFFER_LINES = 10;
    std::vector<std::string> outputBuffer;
    void addOutput(const std::string& line) {
        outputBuffer.push_back(line);
        if (outputBuffer.size() > MAX_BUFFER_LINES) outputBuffer.erase(outputBuffer.begin());
    }
    std::vector<std::string> getRecentOutputs() const { return outputBuffer; }
    void clearRecentOutputs() { outputBuffer.clear(); }

    // for backing store
    std::vector<uint16_t> getPageData(size_t page_number) const;
    void setPagingAllocator(PagingAllocator* allocator) { pagingAllocator = allocator; }
};

#endif // PROCESS_H
