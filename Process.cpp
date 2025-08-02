#include "Process.h"
#include "InstructionGenerator.h"
#include <iostream>
#include <vector>

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

    // Generate random instructions
    InstructionGenerator generator;
    instructions = generator.generateInstructionSet(pname, commands);

    log_file = make_unique<ofstream>("processesLogs/" + name + ".txt");
    if (log_file->is_open()) {
        *log_file << "Process: " << name << endl;
        *log_file << "Logs:" << endl;

        // Log all instructions that will be executed
        *log_file << "Instructions to execute:" << endl;
        for (size_t i = 0; i < instructions.size(); ++i) {
            *log_file << "[" << i << "] " << instructions[i]->toString() << endl;
        }
        *log_file << "Execution log:" << endl;
    }
}

Process::Process(const std::string& pname, const std::vector<std::shared_ptr<Instruction>>& instrs, size_t processMemory, size_t memoryPerFrame)
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

    // Initialize context
    context = make_unique<ProcessContext>(pname);

    log_file = make_unique<ofstream>("processesLogs/" + name + ".txt");
    if (log_file->is_open()) {
        *log_file << "Process: " << name << endl;
        *log_file << "Logs:" << endl;

        // Log all instructions that will be executed
        *log_file << "Instructions to execute:" << endl;
        for (size_t i = 0; i < instructions.size(); ++i) {
            *log_file << "[" << i << "] " << instructions[i]->toString() << endl;
        }
        *log_file << "Execution log:" << endl;
    }

    // INitialize empty pages
    for (size_t i = 0; i < numPages; ++i) {
        page_table[i] = Page{
            .page_number = i, 
            .frame_number = -1,  // not mapped yet
            .inMemory = false,
            .swapped = false
        };
    }
}

Process::~Process() {
    if (log_file && log_file->is_open()) {
        log_file->close();
    }
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
    std::string coreInfo = (status != "Finished") ? " (Core " + std::to_string(core_id.load()) + ")" : "";

    out << std::left << std::setw(10) << name
        << std::setw(25) << ("(" + getFormattedTime() + ")")
        << std::setw(15) << status
        << std::setw(15) << (std::to_string(exec) + "/" + std::to_string(total))
        << std::setw(10) << ("[" + std::to_string(memory) + " KB]")
        << coreInfo
        << "\n";
}

void Process::displayProcess() const {
    displayProcess(std::cout);
}

//void Process::executeCommand(int coreId) {
//    if (isFinished()) return;
//
//    // 1) assign core & advance the context cycle
//    core_id = coreId;
//    context->incrementCycle();
//
//    // 2) handle sleeping exactly as before
//    if (context->isSleeping()) {
//        context->decrementSleep();
//
//        auto now = system_clock::now();
//        time_t t = system_clock::to_time_t(now);
//        tm timeinfo;
//        localtime_s(&timeinfo, &t);
//
//        std::lock_guard<std::mutex> lock(log_mutex);
//        if (log_file && log_file->is_open()) {
//            *log_file
//                << "(" << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S %p") << ") "
//                << "Core:" << coreId << " Process sleeping..."
//                << std::endl;
//        }
//        return;
//    }
//
//    // 3) execute the instruction
//    if (current_instruction < instructions.size()) {
//        bool done = instructions[current_instruction]->execute(*context);
//
//        // timestamp for this cycle
//        auto now = system_clock::now();
//        time_t t = system_clock::to_time_t(now);
//        tm timeinfo;
//        localtime_s(&timeinfo, &t);
//
//        std::lock_guard<std::mutex> lock(log_mutex);
//        if (log_file && log_file->is_open()) {
//            // a) log the “Executing:” line
//            *log_file
//                << "(" << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S %p") << ") "
//                << "Core:" << coreId << " Executing: "
//                << instructions[current_instruction]->toString()
//                << std::endl;
//
//            // b) pull out any PRINT outputs, build full prefix+color line, then log + stash
//            const auto& outputs = context->getOutputBuffer();
//            for (const auto& msg : outputs) {
//                // build timestamp + core prefix
//                std::ostringstream line;
//                // timestamp in orange
//                line << "\x1b[33m("
//                    << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S %p")
//                    << ")\x1b[0m ";
//                // core in cyan
//                line << "\x1b[36mCore:" << coreId << "\x1b[0m ";
//                // message in green (with quotes)
//                line << "\x1b[32m\"" << msg << "\"\x1b[0m";
//
//                // log the colored line
//                *log_file << "    Output: " << line.str() << std::endl;
//
//                // stash into your in-memory buffer (already colorized & timestamped)
//                addOutput(line.str());
//            }
//            // c) clear the Instruction.h buffer
//            context->clearOutputBuffer();
//           
//        }
//
//        // 4) advance your program counter
//        if (done) {
//            executed_commands++;
//            current_instruction++;
//        }
//    }
//}

bool Process::executeCommand(int coreId) {
    if (isFinished()) return true;

    // 1) assign core & advance the context cycle
    core_id = coreId;

    if (instructions[current_instruction]->memoryAccessed()) {
        auto address = instructions[current_instruction]->getMemoryAddress();
        if (address.has_value()) {
            uint16_t address = address.value();
            auto [page_number, offset] = getPageAndOffset(address);

            auto it = page_table.find(page_number);
            if (it == page_table.end() || !it->second.inMemory) {
                return false; // trigger a page fault
            }
        }
    }

    context->incrementCycle();

    // 2) handle sleeping exactly as before
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

    // 3) execute the instruction
    if (current_instruction < instructions.size()) {
        bool done = instructions[current_instruction]->execute(*context);

        // timestamp for this cycle
        auto now = system_clock::now();
        time_t t = system_clock::to_time_t(now);
        tm timeinfo;
        localtime_s(&timeinfo, &t);

        std::lock_guard<std::mutex> lock(log_mutex);
        if (log_file && log_file->is_open()) {
            // a) log the “Executing:” line
            *log_file
                << "(" << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S %p") << ") "
                << "Core:" << coreId << " Executing: "
                << instructions[current_instruction]->toString()
                << std::endl;

            // b) pull out any PRINT outputs, build full prefix+color line, then log + stash
            const auto& outputs = context->getOutputBuffer();
            for (const auto& msg : outputs) {
                // build timestamp + core prefix
                std::ostringstream line;
                // timestamp in orange
                line << "\x1b[33m("
                    << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S %p")
                    << ")\x1b[0m ";
                // core in cyan
                line << "\x1b[36mCore:" << coreId << "\x1b[0m ";
                // message in green (with quotes)
                line << "\x1b[32m\"" << msg << "\"\x1b[0m";

                // log the colored line
                *log_file << "    Output: " << line.str() << std::endl;

                // stash into your in-memory buffer (already colorized & timestamped)
                addOutput(line.str());
            }
            // c) clear the Instruction.h buffer
            context->clearOutputBuffer();

        }

        // 4) advance your program counter
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
    if (core >= 0) {
        return "Core " + to_string(core);
    }
    return "Not assigned";
}

void Process::setInstructions(const std::vector<std::shared_ptr<Instruction>>& instrs) {
    instructions = instrs;
    total_commands = static_cast<int>(instrs.size());
    current_instruction = 0;
    executed_commands = 0;
}

void Process::displayProcessInfo() const {
    cout << "Process: " << name << endl;
    cout << "ID: " << process_id << endl;
    cout << "Current instruction line: " << current_instruction << endl;
    cout << "Lines of code: " << total_commands << endl;

    if (isFinished()) {
        cout << "Finished!" << endl;
    }
    else {
        cout << "Status: " << getStatus() << endl;

        // Show current instruction if available
        if (current_instruction < instructions.size()) {
            cout << "Current instruction: " << instructions[current_instruction]->toString() << endl;
        }

    }
    cout << endl;
}

//void Process::setMemBlock(void* block) {
//    memBlock = block;
//}
//
//void* Process::getMemBlock() const {
//    return memBlock;
//}
//
