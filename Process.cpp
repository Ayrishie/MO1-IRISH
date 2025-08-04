#include "Process.h"
#include "InstructionGenerator.h"
#include "Instruction.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <fstream>
#include <utility>   // std::pair

using namespace std;
using namespace std::chrono;

// Initialize static members
atomic<int> Process::next_process_id = 0;
mutex Process::id_mutex;
mutex Process::log_mutex;

Process::Process(const std::string& pname, int commands, size_t processMemory)
    : name(pname),
    total_commands(commands),
    executed_commands(0),
    core_id(-1),
    processMemory(processMemory),
    start_time(system_clock::now()),
    current_instruction(0)
{
    lock_guard<mutex> lock(id_mutex);
    process_id = next_process_id++;

    // Initialize context
    context = make_unique<ProcessContext>(pname);

    // Use per-process memory to seed generator for safe address ranges
    InstructionGenerator generator(static_cast<int>(processMemory));
    instructions = generator.generateInstructionSet(pname, commands);

    // No paging for randomly-generated processes by default
    pageSize = 0;
    numPages = 0;
    page_table.clear();

    log_file = make_unique<ofstream>("processesLogs/" + name + ".txt");
    if (log_file->is_open()) {
        *log_file << "Process: " << name << "\n";
        *log_file << "Logs:\n";
        *log_file << "Instructions to execute:\n";
        for (size_t i = 0; i < instructions.size(); ++i) {
            *log_file << "[" << i << "] " << instructions[i]->toString() << "\n";
        }
        *log_file << "Execution log:\n";
    }
}

Process::Process(const std::string& pname,
    const std::vector<std::shared_ptr<Instruction>>& instrs,
    size_t processMemory,
    size_t memoryPerFrame)
    : name(pname),
    total_commands(static_cast<int>(instrs.size())),
    executed_commands(0),
    core_id(-1),
    processMemory(processMemory),
    start_time(system_clock::now()),
    current_instruction(0),
    instructions(instrs),
    pageSize(memoryPerFrame),
    numPages((processMemory + memoryPerFrame - 1) / memoryPerFrame)
{
    lock_guard<mutex> lock(id_mutex);
    process_id = next_process_id++;

    context = make_unique<ProcessContext>(pname);

    log_file = make_unique<ofstream>("processesLogs/" + name + ".txt");
    if (log_file->is_open()) {
        *log_file << "Process: " << name << "\n";
        *log_file << "Logs:\n";
        *log_file << "Instructions to execute:\n";
        for (size_t i = 0; i < instructions.size(); ++i) {
            *log_file << "[" << i << "] " << instructions[i]->toString() << "\n";
        }
        *log_file << "Execution log:\n";
    }

    // Initialize empty page table
    page_table.clear();
    for (size_t i = 0; i < numPages; ++i) {
        Page p{};
        p.page_number = i;     // size_t
        p.frame_number = -1;   // not mapped yet
        p.inMemory = false;
        p.swapped = false;
        page_table.emplace(i, p);
    }
}

Process::~Process() {
    if (log_file && log_file->is_open()) {
        log_file->close();
    }
}

// ---- matches the declaration in the header; no inline body in the header ----
std::pair<size_t, size_t> Process::getPageAndOffset(uint16_t addr) const {
    if (pageSize == 0) {
        return { 0u, static_cast<size_t>(addr) };
    }
    return { static_cast<size_t>(addr) / pageSize,
             static_cast<size_t>(addr) % pageSize };
}

string Process::getFormattedTime() const {
    time_t st = system_clock::to_time_t(start_time);
    struct tm timeinfo;
    localtime_s(&timeinfo, &st);
    stringstream ss;
    ss << put_time(&timeinfo, "%m/%d/%Y %I:%M:%S %p");
    return ss.str();
}

string Process::getStatus() const {
    if (executed_commands.load() == total_commands) {
        return "Finished";
    }
    return getCoreAssignment();
}

std::string Process::getName() const {
    return name;
}

void Process::displayProcess(std::ostream& out) const {
    int exec = executed_commands.load();
    int total = total_commands;
    std::string status = getStatus();
    std::string coreInfo = (status != "Finished")
        ? " (Core " + std::to_string(core_id.load()) + ")"
        : "";

    out << std::left << std::setw(10) << name
        << std::setw(25) << ("(" + getFormattedTime() + ")")
        << std::setw(15) << status
        << std::setw(15) << (std::to_string(exec) + "/" + std::to_string(total))
        << std::setw(14) << ("[" + std::to_string(processMemory) + " bytes]")
        << coreInfo
        << "\n";
}

void Process::displayProcess() const {
    displayProcess(std::cout);     // call the 1-arg overload
}

/*
 * Helper: ensure a given logical address is resident in our page table.
 * Lightweight “lazy map” for now.
 */
void Process::ensureResident(uint16_t address) {
    if (pageSize == 0 || numPages == 0) return;

    // Correct structured binding (pair<size_t,size_t>)
    auto [page_number, offset] = getPageAndOffset(address);
    (void)offset;

    // page_table is unordered_map<size_t, Page>
    auto it = page_table.find(page_number);
    if (it == page_table.end()) {
        Page p{};
        p.page_number = page_number;  // size_t
        p.frame_number = -1;
        p.inMemory = false;
        p.swapped = false;
        it = page_table.emplace(page_number, p).first; // OK: key is size_t
    }

    Page& pg = it->second;
    if (!pg.inMemory) {
        pg.frame_number = static_cast<int>(page_number % 1024); // placeholder
        pg.inMemory = true;
        pg.swapped = false;

        if (log_file && log_file->is_open()) {
            *log_file << "    [paging] mapped page " << page_number
                << " -> frame " << pg.frame_number << "\n";
        }
    }
}

bool Process::executeCommand(int coreId) {
    if (isFinished()) return true;

    core_id = coreId;
    context->incrementCycle();

    if (current_instruction < instructions.size()) {
        auto instr = instructions[current_instruction];
        if (instr->memoryAccessed()) {
            uint16_t address = 0;
            if (auto rd = dynamic_cast<ReadInstruction*>(instr.get())) {
                address = rd->getMemoryAddress();
            }
            else if (auto wr = dynamic_cast<WriteInstruction*>(instr.get())) {
                address = wr->getMemoryAddress();
            }
            ensureResident(address);
        }
    }

    if (context->isSleeping()) {
        context->decrementSleep();

        auto now = system_clock::now();
        time_t t = system_clock::to_time_t(now);
        tm timeinfo;
        localtime_s(&timeinfo, &t);

        std::lock_guard<std::mutex> lock(log_mutex);
        if (log_file && log_file->is_open()) {
            *log_file
                << "(" << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S %p") << ") "
                << "Core:" << coreId << " Process sleeping..."
                << std::endl;
        }
        return true;
    }

    if (current_instruction < instructions.size()) {
        bool done = instructions[current_instruction]->execute(*context);

        auto now = system_clock::now();
        time_t t = system_clock::to_time_t(now);
        tm timeinfo;
        localtime_s(&timeinfo, &t);

        std::lock_guard<std::mutex> lock(log_mutex);
        if (log_file && log_file->is_open()) {
            *log_file
                << "(" << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S %p") << ") "
                << "Core:" << coreId << " Executing: "
                << instructions[current_instruction]->toString()
                << std::endl;

            const auto& outputs = context->getOutputBuffer();
            for (const auto& msg : outputs) {
                std::ostringstream line;
                line << "\x1b[33m(" << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S %p")
                    << ")\x1b[0m ";
                line << "\x1b[36mCore:" << coreId << "\x1b[0m ";
                line << "\x1b[32m\"" << msg << "\"\x1b[0m";

                *log_file << "    Output: " << line.str() << std::endl;
                addOutput(line.str());
            }
            context->clearOutputBuffer();
        }

        if (done) {
            executed_commands++;
            current_instruction++;
        }
    }
    return true;
}

bool Process::isFinished() const {
    return current_instruction >= instructions.size() && !context->isSleeping();
}

string Process::getCoreAssignment() const {
    int core = core_id.load();
    if (core >= 0) return "Core " + to_string(core);
    return "Not assigned";
}

void Process::setInstructions(const std::vector<std::shared_ptr<Instruction>>& instrs) {
    instructions = instrs;
    total_commands = static_cast<int>(instrs.size());
    current_instruction = 0;
    executed_commands = 0;
}

void Process::displayProcessInfo() const {
    std::cout << "Process: " << name << '\n';
    std::cout << "ID: " << process_id << '\n';
    std::cout << "Current instruction line: " << current_instruction << '\n';
    std::cout << "Lines of code: " << total_commands << '\n';

    if (isFinished()) {
        std::cout << "Finished!\n\n";
        return;
    }

    std::cout << "Status: " << getStatus() << '\n';
    const int sz = static_cast<int>(instructions.size());
    if (current_instruction >= 0 && current_instruction < sz) {
        std::cout << "Current instruction: "
            << instructions[static_cast<size_t>(current_instruction)]->toString()
            << '\n';
    }
    std::cout << '\n';
}
