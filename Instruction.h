#pragma once
#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <sstream>
#include <iostream>   // <-- add this
#include "MemoryAllocator.h"

// 0=PRINT, 1=DECLARE, 2=ADD, 3=SUBTRACT, 4=SLEEP, 5=FOR, 6=READ, 7=WRITE
#define PRINT     0
#define DECLARE   1
#define ADD       2
#define SUBTRACT  3
#define SLEEP     4
#define FOR       5
#define READ      6
#define WRITE     7

// Forward declaration
class ProcessContext;

// --- DEBUG helper: escape non-printables for logs (TEMPORARY) ---
inline std::string dbgEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() * 2);
    for (unsigned char c : s) {
        switch (c) {
        case '\0': out += "\\0"; break;
        case '\n': out += "\\n"; break;
        case '\t': out += "\\t"; break;
        case '\\': out += "\\\\"; break;
        case '\"': out += "\\\""; break;
        default:   out += static_cast<char>(c); break;
        }
    }
    return out;
}


// -------------------- helpers --------------------

inline std::string trimVar(std::string s) {
    auto sp = [](unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!s.empty() && sp((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && sp((unsigned char)s.back()))  s.pop_back();
    return s;
}

// Unescape \" \\ \n \t and strip wrapping quotes, then trim
inline std::string unescapeAndStripQuotes(std::string s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '\\' && i + 1 < s.size()) {
            char n = s[++i];
            switch (n) {
            case '\\': out.push_back('\\'); break;
            case '"':  out.push_back('"');  break;
            case 'n':  out.push_back('\n'); break;
            case 't':  out.push_back('\t'); break;
            default:   out.push_back(n);    break;
            }
        }
        else {
            out.push_back(c);
        }
    }
    if (out.size() >= 2 && out.front() == '"' && out.back() == '"') {
        out = out.substr(1, out.size() - 2);
    }
    return trimVar(std::move(out));
}

// -------------------- base instruction --------------------

class Instruction {
protected:
    int type;
public:
    virtual ~Instruction() = default;
    virtual bool execute(ProcessContext& context) = 0;  // returns true if instruction completed
    virtual std::string toString() const = 0;
    virtual int getExecutionCycles() const { return 1; }
    virtual int getInstructionType() const { return type; }
    virtual bool memoryAccessed() const { return false; }
};

// -------------------- process context --------------------

class ProcessContext {
private:
    static constexpr size_t MAX_VARIABLES = 32;  // 64 bytes / 2 bytes per uint16_t
    std::unordered_map<std::string, uint16_t> variables;
    std::string processName;
    int currentCycle;
    int sleepCycles;
    std::vector<std::string> outputBuffer;
    std::unordered_map<uint16_t, uint16_t> memory;        // simulated memory
public:
    ProcessContext(const std::string& name)
        : processName(name), currentCycle(0), sleepCycles(0) {
    }

    uint16_t getVariable(const std::string& name) {
        auto it = variables.find(name);
        if (it != variables.end()) {
            return it->second;
        }

        // Check if we can add a new variable
        if (variables.size() >= MAX_VARIABLES) {
            std::cerr << "\033[31m[ERROR] Symbol table full - cannot create variable '"
                << name << "' (max " << MAX_VARIABLES << " variables)\033[0m\n";
            return 0;
        }

        // Auto-declare with 0
        variables[name] = 0;
        return 0;
    }


    void setVariable(const std::string& name, uint16_t value) {
        if (variables.find(name) == variables.end() && variables.size() >= MAX_VARIABLES) {
            std::cerr << "\033[31m[ERROR] Symbol table full - cannot create variable '"
                << name << "' (max " << MAX_VARIABLES << " variables)\033[0m\n";
            return;
        }
        variables[name] = value;
    }
    // Sleep
    void setSleep(int cycles) { sleepCycles = cycles; }
    bool isSleeping() const { return sleepCycles > 0; }
    void decrementSleep() { if (sleepCycles > 0) --sleepCycles; }

    // Output
    void addOutput(const std::string& output) { outputBuffer.push_back(output); }
    const std::vector<std::string>& getOutputBuffer() const { return outputBuffer; }
    void clearOutputBuffer() { outputBuffer.clear(); }

    // Cycles
    const std::string& getProcessName() const { return processName; }
    int getCurrentCycle() const { return currentCycle; }
    void incrementCycle() { ++currentCycle; }

    // Memory
    void writeMemory(uint16_t address, uint16_t value) { memory[address] = value; }
    uint16_t readMemory(uint16_t address) const {
        auto it = memory.find(address);
        return (it != memory.end()) ? it->second : 0;
    }
};

// -------------------- PRINT --------------------

class PrintInstruction : public Instruction {
private:
    std::string message;
    std::string variable;   // optional variable to append
    bool hasVariable;
public:
    // print("literal")
    PrintInstruction(const std::string& msg)
        : message(unescapeAndStripQuotes(msg)), hasVariable(false) {
        type = PRINT;
    }
    // print("literal" + var)
    PrintInstruction(const std::string& msg, const std::string& var)
        : message(unescapeAndStripQuotes(msg)),
        variable(trimVar(var)),
        hasVariable(true) {
        type = PRINT;
    }

    bool execute(ProcessContext& context) override {
        // TEMP DEBUG: show exactly what message and variable look like here
        if (hasVariable) {
            uint16_t value = context.getVariable(variable);
            context.addOutput(std::string("[DBG] msg=") + dbgEscape(message)
                + ", var=" + variable
                + ", val=" + std::to_string(value));
            // normal output
            context.addOutput(message + std::to_string(value));
        }
        else {
            context.addOutput(std::string("[DBG] msg=") + dbgEscape(message));
            context.addOutput(message);
        }
        return true;
    }


    std::string toString() const override {
        if (hasVariable) return "PRINT(\"" + message + "\" + " + variable + ")";
        return "PRINT(\"" + message + "\")";
    }
};

// -------------------- DECLARE --------------------

class DeclareInstruction : public Instruction {
private:
    std::string variableName;
    uint16_t value;
public:
    DeclareInstruction(const std::string& var, uint16_t val)
        : variableName(trimVar(var)), value(val) {
        type = DECLARE;
    }
    bool execute(ProcessContext& context) override {
        context.setVariable(variableName, value);
        return true;
    }
    std::string toString() const override {
        return "DECLARE(" + variableName + ", " + std::to_string(value) + ")";
    }
};

// -------------------- ADD --------------------

class AddInstruction : public Instruction {
private:
    std::string result;
    std::string operand1;
    std::string operand2;
    bool op1IsValue = false;
    bool op2IsValue = false;
    uint16_t op1Value = 0;
    uint16_t op2Value = 0;
public:
    AddInstruction(const std::string& res, const std::string& op1, const std::string& op2)
        : result(trimVar(res)), operand1(trimVar(op1)), operand2(trimVar(op2)) {
        type = ADD;
    }
    AddInstruction(const std::string& res, const std::string& op1, uint16_t op2)
        : result(trimVar(res)), operand1(trimVar(op1)), op2IsValue(true), op2Value(op2) {
        type = ADD;
    }
    AddInstruction(const std::string& res, uint16_t op1, const std::string& op2)
        : result(trimVar(res)), operand2(trimVar(op2)), op1IsValue(true), op1Value(op1) {
        type = ADD;
    }
    AddInstruction(const std::string& res, uint16_t op1, uint16_t op2)
        : result(trimVar(res)), op1IsValue(true), op2IsValue(true), op1Value(op1), op2Value(op2) {
        type = ADD;
    }

    bool execute(ProcessContext& context) override {
        uint16_t v1 = op1IsValue ? op1Value : context.getVariable(operand1);
        uint16_t v2 = op2IsValue ? op2Value : context.getVariable(operand2);
        uint32_t sum = static_cast<uint32_t>(v1) + static_cast<uint32_t>(v2);
        uint16_t rv = (sum > UINT16_MAX) ? UINT16_MAX : static_cast<uint16_t>(sum);
        context.setVariable(result, rv);
        return true;
    }
    std::string toString() const override {
        std::string a = op1IsValue ? std::to_string(op1Value) : operand1;
        std::string b = op2IsValue ? std::to_string(op2Value) : operand2;
        return "ADD(" + result + ", " + a + ", " + b + ")";
    }
};

// -------------------- SUBTRACT --------------------

class SubtractInstruction : public Instruction {
private:
    std::string result;
    std::string operand1;
    std::string operand2;
    bool op1IsValue = false;
    bool op2IsValue = false;
    uint16_t op1Value = 0;
    uint16_t op2Value = 0;
public:
    SubtractInstruction(const std::string& res, const std::string& op1, const std::string& op2)
        : result(trimVar(res)), operand1(trimVar(op1)), operand2(trimVar(op2)) {
        type = SUBTRACT;
    }
    SubtractInstruction(const std::string& res, const std::string& op1, uint16_t op2)
        : result(trimVar(res)), operand1(trimVar(op1)), op2IsValue(true), op2Value(op2) {
        type = SUBTRACT;
    }
    SubtractInstruction(const std::string& res, uint16_t op1, const std::string& op2)
        : result(trimVar(res)), operand2(trimVar(op2)), op1IsValue(true), op1Value(op1) {
        type = SUBTRACT;
    }
    SubtractInstruction(const std::string& res, uint16_t op1, uint16_t op2)
        : result(trimVar(res)), op1IsValue(true), op2IsValue(true), op1Value(op1), op2Value(op2) {
        type = SUBTRACT;
    }

    bool execute(ProcessContext& context) override {
        uint16_t v1 = op1IsValue ? op1Value : context.getVariable(operand1);
        uint16_t v2 = op2IsValue ? op2Value : context.getVariable(operand2);
        uint16_t rv = (v1 >= v2) ? (v1 - v2) : 0;
        context.setVariable(result, rv);
        return true;
    }
    std::string toString() const override {
        std::string a = op1IsValue ? std::to_string(op1Value) : operand1;
        std::string b = op2IsValue ? std::to_string(op2Value) : operand2;
        return "SUBTRACT(" + result + ", " + a + ", " + b + ")";
    }
};

// -------------------- SLEEP --------------------

class SleepInstruction : public Instruction {
private:
    uint8_t cycles;
public:
    explicit SleepInstruction(uint8_t c) : cycles(c) { type = SLEEP; }
    bool execute(ProcessContext& context) override { context.setSleep(cycles); return true; }
    std::string toString() const override { return "SLEEP(" + std::to_string(cycles) + ")"; }
    int getExecutionCycles() const override { return cycles; }
};

// -------------------- FOR --------------------

class ForInstruction : public Instruction {
private:
    std::vector<std::shared_ptr<Instruction>> instructions;
    int repeats;
    mutable int currentIteration;
    mutable int currentInstructionIndex;
public:
    ForInstruction(const std::vector<std::shared_ptr<Instruction>>& instrs, int reps)
        : instructions(instrs), repeats(reps), currentIteration(0), currentInstructionIndex(0) {
        type = FOR;
    }
    bool execute(ProcessContext& context) override {
        if (currentIteration >= repeats) return true;
        if (currentInstructionIndex >= static_cast<int>(instructions.size())) {
            ++currentIteration; currentInstructionIndex = 0;
            if (currentIteration >= repeats) return true;
        }
        if (currentInstructionIndex < static_cast<int>(instructions.size())) {
            if (instructions[currentInstructionIndex]->execute(context)) {
                ++currentInstructionIndex;
            }
        }
        return false;
    }
    std::string toString() const override {
        return "FOR([" + std::to_string(instructions.size()) + " instructions], " +
            std::to_string(repeats) + ")";
    }
};

// -------------------- WRITE --------------------

class WriteInstruction : public Instruction {
private:
    uint16_t address;
    std::string variable;
public:
    WriteInstruction(uint16_t addr, const std::string& var)
        : address(addr), variable(trimVar(var)) {
        type = WRITE;
    }
    bool execute(ProcessContext& context) override {
        context.writeMemory(address, context.getVariable(variable));
        return true;
    }
    std::string toString() const override {
        std::stringstream ss; ss << "WRITE(0x" << std::hex << address << ", " << variable << ")";
        return ss.str();
    }
    bool memoryAccessed() const override { return true; }
    uint16_t getMemoryAddress() const { return address; }
};

// -------------------- READ --------------------

class ReadInstruction : public Instruction {
private:
    std::string variable;
    uint16_t address;
public:
    ReadInstruction(const std::string& var, uint16_t addr)
        : variable(trimVar(var)), address(addr) {
        type = READ;
    }
    bool execute(ProcessContext& context) override {
        context.setVariable(variable, context.readMemory(address));
        return true;
    }
    std::string toString() const override {
        std::stringstream ss; ss << "READ(" << variable << ", 0x" << std::hex << address << ")";
        return ss.str();
    }
    bool memoryAccessed() const override { return true; }
    uint16_t getMemoryAddress() const { return address; }
};

#endif // INSTRUCTION_H
