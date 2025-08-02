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
#include "Instruction.h"

// Page Data Structure
// Virtual Addressing Scheme
typedef struct Page {
    size_t page_number;   // process page number
    size_t frame_number;  // actual physical memory frame number
    bool inMemory;     // Flag to track if Page is loaded into RAM
    bool swapped;      // Flag to detect if Page has been swapped with the backing store
} Page;

class Process {
private:
    static std::atomic<int> next_process_id;
    static std::mutex id_mutex;
    std::unique_ptr<std::ofstream> log_file;
    static std::mutex log_mutex; 

    bool debug = true;  // toggle debug on/off

    // Instruction-related members
    std::vector<std::shared_ptr<Instruction>> instructions;

    std::unique_ptr<ProcessContext> context;

    int current_instruction;

    // ——— Rolling PRINT/debug buffer ———
    static constexpr size_t MAX_BUFFER_LINES = 10;
    std::vector<std::string> outputBuffer;

    // std::unordered_map<std::string, uint16_t> symbolTable; // MAX of 32 variables {variable name string -> maps to -> 2 byte value)

public:
    std::string name;
    int total_commands;
    std::atomic<int> executed_commands;
    std::chrono::time_point<std::chrono::system_clock> start_time;
    std::atomic<int> core_id;  
    int process_id;
    size_t processMemory; // renamed from "memory"

    // NEW ATTRIBUTES
    std::vector<Page> pageTable; // per-process page list
    size_t pageSize;             // Size of each page in memory: `mem-per-frame` config variable
    size_t numPages;             // Number of pages: `max-overall-mem` / `mem-per-frame` config variables

    std::unordered_map<size_t, Page> page_table; // a virtual address -> page mapping might be more efficient to use

    Process(const std::string& pname, int commands, size_t memory);
    Process(const std::string& pname,
        const std::vector<std::shared_ptr<Instruction>>& instrs,
        size_t processMemory,
        size_t memoryPerFrame);
    ~Process();

    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    // Original methods
    std::string getFormattedTime() const;
    std::string getStatus() const;
    std::string getName() const;
    void displayProcess() const;
    bool isFinished() const;
    std::string getCoreAssignment() const;

    // Updated execution method
    bool executeCommand(int coreId);

    // Instruction-set management
    void setInstructions(const std::vector<std::shared_ptr<Instruction>>& instrs);
    const std::vector<std::shared_ptr<Instruction>>& getInstructions() const {
        return instructions;
    }


    /// Returns the zero-based index of the next instruction to execute. NEW
    int getCurrentInstructionLine() const {
        return current_instruction;
    }

    std::shared_ptr<Instruction> getNextInstruction() const {
        return this->instructions[current_instruction];
    }

    // Methods for screen command support
    void displayProcessInfo() const;
    void displayProcess(std::ostream& out) const;

    // ——— Rolling buffer API ———
        /// Append one line to our in-memory buffer (auto-trims to last N)
    void addOutput(const std::string& line) {
        outputBuffer.push_back(line);
        if (outputBuffer.size() > MAX_BUFFER_LINES) {
            outputBuffer.erase(outputBuffer.begin());
        }
    }
    /// Get a copy of buffered lines
    std::vector<std::string> getRecentOutputs() const {
        return outputBuffer;
    }
    /// Clear the buffer
    void clearRecentOutputs() {
        outputBuffer.clear();
    }

    std::pair<size_t, size_t> getPageAndOffset(uint16_t virtualAddress) const {
        return { virtualAddress / pageSize, virtualAddress % pageSize };
    }
};

#endif // PROCESS_H
