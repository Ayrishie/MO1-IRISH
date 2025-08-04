#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <mutex>
#include <memory>
#include <vector>
#include <ostream>
#include <unordered_map>
#include <cstdint>

#include "Instruction.h"   // uses Instruction and ProcessContext types

// ---------- Paging data ----------
struct Page {
    std::size_t page_number = 0;  // logical page index
    int         frame_number = -1; // physical frame id; -1 if not mapped
    bool        inMemory = false; // resident in RAM?
    bool        swapped = false; // swapped out?
};

class PagingAllocator;   // forward declaration
class ProcessContext;    // forward declaration

class Process {
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
    std::size_t pageSize = 0;                                // bytes per page (frame size)
    std::size_t numPages = 0;                                // total pages for this proc

    // *** SINGLE definition of the page table ***
    std::unordered_map<std::size_t, Page> page_table;        // page_number -> Page

    // internal helper to ensure a page is resident
    void ensureResident(uint16_t address);


    //memory issues
    bool memoryViolation = false;
    std::string violationTime;
    uint16_t violationAddress = 0;

public:
    // ---- public identity/state ----
    std::string name;
    int         total_commands = 0;
    std::atomic<int> executed_commands{ 0 };
    std::chrono::time_point<std::chrono::system_clock> start_time;
    std::atomic<int> core_id{ -1 };
    int         process_id = -1;
    std::size_t processMemory = 0;   // bytes

    // ---- ctors/dtor ----
    Process(const std::string& pname, int commands, std::size_t memory);
    Process(const std::string& pname,
        const std::vector<std::shared_ptr<Instruction>>& instrs,
        std::size_t processMemory,
        std::size_t memoryPerFrame);
    ~Process();

    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    // ---- API ----
    std::string getFormattedTime() const;
    std::string getStatus() const;
    std::string getName() const;
    void        displayProcess() const;
    void        displayProcess(std::ostream& out) const;
    void        displayProcessInfo() const;
    bool        isFinished() const;
    std::string getCoreAssignment() const;

    // make the mapper usable by the scheduler
    std::pair<std::size_t, std::size_t> getPageAndOffset(uint16_t addr) const;

    bool        executeCommand(int coreId);

    void setInstructions(const std::vector<std::shared_ptr<Instruction>>& instrs);
    const std::vector<std::shared_ptr<Instruction>>& getInstructions() const { return instructions; }

    int  getCurrentInstructionLine() const { return current_instruction; }
    std::shared_ptr<Instruction> getNextInstruction() const;

    // rolling buffer for colored PRINT lines
    static constexpr std::size_t MAX_BUFFER_LINES = 10;
    std::vector<std::string> outputBuffer;
    void addOutput(const std::string& line) {
        outputBuffer.push_back(line);
        if (outputBuffer.size() > MAX_BUFFER_LINES) outputBuffer.erase(outputBuffer.begin());
    }
    std::vector<std::string> getRecentOutputs() const { return outputBuffer; }
    void clearRecentOutputs() { outputBuffer.clear(); }

    // expose read/write access to the page table (do NOT redeclare the member)
    using PageTable = std::unordered_map<std::size_t, Page>;
    PageTable& getPageTable() { return page_table; }
    const PageTable& getPageTable() const { return page_table; }



    // Memory violation getters
    bool hasMemoryViolation() const { return memoryViolation; }
    std::string getViolationTime() const { return violationTime; }
    uint16_t getViolationAddress() const { return violationAddress; }

};

#endif // PROCESS_H
