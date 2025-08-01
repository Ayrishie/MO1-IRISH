#pragma once
#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "MemoryAllocator.h"

// 0=PRINT, 1=DECLARE, 2=ADD, 3=SUBTRACT, 4=SLEEP, 5=FOR, 6=READ, 7=WRITE

#define PRINT     0;
#define DECLARE   1;
#define ADD       2;
#define SUBTRACT  3;
#define SLEEP     4;
#define FOR       5;
#define READ      6;
#define WRITE     7;

// Forward declaration
class ProcessContext;

// Base instruction interface
class Instruction {
protected:
    int type;

public:
    virtual ~Instruction() = default;
    virtual bool execute(ProcessContext& context) = 0;  // Returns true if instruction completed
    virtual std::string toString() const = 0;
    virtual int getExecutionCycles() const { return 1; }  // Default: 1 cycle per instruction
    virtual int getInstructionType() const { return type; }

    virtual bool memoryAccessed() const { return false; }
    virtual std::optional<uint16_t> getAddress() const { return std::nullopt; }
};

// Process context to hold variables and state
class ProcessContext {
private:
    std::unordered_map<std::string, uint16_t> variables; // Simulated symbol table
    std::string processName;
    int currentCycle;
    int sleepCycles;
    std::vector<std::string> outputBuffer;  // To store PRINT outputs
    // NEW
    std::unordered_map<uint16_t, uint16_t> memory;  // Simulated memory

public:
    ProcessContext(const std::string& name) 
        : processName(name), currentCycle(0), sleepCycles(0) {}

   uint16_t getVariable(const std::string& name) {
        auto it = variables.find(name);
        return (it != variables.end()) ? it->second : 0;  // Auto-declare with 0 if not found
    }

    void setVariable(const std::string& name, uint16_t value) {
        variables[name] = value;
    }

    // Sleep management
    void setSleep(int cycles) { sleepCycles = cycles; }
    bool isSleeping() const { return sleepCycles > 0; }
    void decrementSleep() { if (sleepCycles > 0) sleepCycles--; }

    // Output management
    void addOutput(const std::string& output) { outputBuffer.push_back(output); }
    const std::vector<std::string>& getOutputBuffer() const { return outputBuffer; }
    void clearOutputBuffer() { outputBuffer.clear(); }

    // Getters
    const std::string& getProcessName() const { return processName; }
    int getCurrentCycle() const { return currentCycle; }
    void incrementCycle() { currentCycle++; }

    // Simulated memory access
    void writeMemory(uint16_t address, uint16_t value) {
        memory[address] = value;
    }

    uint16_t readMemory(uint16_t address) const {
        auto it = memory.find(address);
        return (it != memory.end()) ? it->second : 0;
    }
};  // Variable management
   

// PRINT instruction
class PrintInstruction : public Instruction {
private:
    std::string message;
    std::string variable;  // Optional variable to print
    bool hasVariable;

public:
    PrintInstruction(const std::string& msg) 
        : message(msg), hasVariable(false), type(PRINT) {
    }

    PrintInstruction(const std::string& msg, const std::string& var)
        : message(msg), variable(var), hasVariable(true), type(PRINT) {
    }

    bool execute(ProcessContext& context) override {
        std::string output;
        if (hasVariable) {
            uint16_t value = context.getVariable(variable);
            output = message + std::to_string(value);
        }
        else {
            output = message;
        }
        context.addOutput(output);
        return true;  // Always completes in one cycle
    }

    std::string toString() const override {
        if (hasVariable) {
            return "PRINT(\"" + message + "\" + " + variable + ")";
        }
        return "PRINT(\"" + message + "\")";
    }
};

// DECLARE instruction
class DeclareInstruction : public Instruction {
private:
    std::string variableName;
    uint16_t value;

public:
    DeclareInstruction(const std::string& var, uint16_t val)
        : variableName(var), value(val), type(DECLARE) {
    }

    bool execute(ProcessContext& context) override {
        context.setVariable(variableName, value);
        return true;
    }

    std::string toString() const override {
        return "DECLARE(" + variableName + ", " + std::to_string(value) + ")";
    }
};

// ADD instruction
class AddInstruction : public Instruction {
private:
    std::string result;
    std::string operand1;
    std::string operand2;
    bool op1IsValue;
    bool op2IsValue;
    uint16_t op1Value;
    uint16_t op2Value;

public:
    AddInstruction(const std::string& res, const std::string& op1, const std::string& op2)
        : result(res), operand1(op1), operand2(op2), op1IsValue(false), op2IsValue(false), type(ADD) {
    }

    AddInstruction(const std::string& res, const std::string& op1, uint16_t op2)
        : result(res), operand1(op1), op2Value(op2), op1IsValue(false), op2IsValue(true), type(ADD) {
    }

    AddInstruction(const std::string& res, uint16_t op1, const std::string& op2)
        : result(res), operand2(op2), op1Value(op1), op1IsValue(true), op2IsValue(false), type(ADD) {
    }

    AddInstruction(const std::string& res, uint16_t op1, uint16_t op2)
        : result(res), op1Value(op1), op2Value(op2), op1IsValue(true), op2IsValue(true), type(ADD) {
    }

    bool execute(ProcessContext& context) override {
        uint16_t val1 = op1IsValue ? op1Value : context.getVariable(operand1);
        uint16_t val2 = op2IsValue ? op2Value : context.getVariable(operand2);

        // Perform addition with overflow protection
        uint32_t sum = static_cast<uint32_t>(val1) + static_cast<uint32_t>(val2);
        uint16_t resultValue = (sum > UINT16_MAX) ? UINT16_MAX : static_cast<uint16_t>(sum);

        context.setVariable(result, resultValue);
        return true;
    }

    std::string toString() const override {
        std::string op1Str = op1IsValue ? std::to_string(op1Value) : operand1;
        std::string op2Str = op2IsValue ? std::to_string(op2Value) : operand2;
        return "ADD(" + result + ", " + op1Str + ", " + op2Str + ")";
    }
};

// SUBTRACT instruction
class SubtractInstruction : public Instruction {
private:
    std::string result;
    std::string operand1;
    std::string operand2;
    bool op1IsValue;
    bool op2IsValue;
    uint16_t op1Value;
    uint16_t op2Value;

public:
    SubtractInstruction(const std::string& res, const std::string& op1, const std::string& op2)
        : result(res), operand1(op1), operand2(op2), op1IsValue(false), op2IsValue(false), type(SUBTRACT) {
    }

    SubtractInstruction(const std::string& res, const std::string& op1, uint16_t op2)
        : result(res), operand1(op1), op2Value(op2), op1IsValue(false), op2IsValue(true), type(SUBTRACT) {
    }

    SubtractInstruction(const std::string& res, uint16_t op1, const std::string& op2)
        : result(res), operand2(op2), op1Value(op1), op1IsValue(true), op2IsValue(false), type(SUBTRACT) {
    }

    SubtractInstruction(const std::string& res, uint16_t op1, uint16_t op2)
        : result(res), op1Value(op1), op2Value(op2), op1IsValue(true), op2IsValue(true), type(SUBTRACT) {
    }

    bool execute(ProcessContext& context) override {
        uint16_t val1 = op1IsValue ? op1Value : context.getVariable(operand1);
        uint16_t val2 = op2IsValue ? op2Value : context.getVariable(operand2);

        // Perform subtraction with underflow protection (clamp to 0)
        uint16_t resultValue = (val1 >= val2) ? (val1 - val2) : 0;

        context.setVariable(result, resultValue);
        return true;
    }

    std::string toString() const override {
        std::string op1Str = op1IsValue ? std::to_string(op1Value) : operand1;
        std::string op2Str = op2IsValue ? std::to_string(op2Value) : operand2;
        return "SUBTRACT(" + result + ", " + op1Str + ", " + op2Str + ")";
    }
};

// SLEEP instruction
class SleepInstruction : public Instruction {
private:
    uint8_t cycles;

public:
    SleepInstruction(uint8_t c) : cycles(c) {}

    bool execute(ProcessContext& context) override {
        context.setSleep(cycles);
        return true;  // Sleep instruction itself completes immediately
    }

    std::string toString() const override {
        return "SLEEP(" + std::to_string(cycles) + ")";
    }

    int getExecutionCycles() const override { return cycles; }
};

// FOR instruction
class ForInstruction : public Instruction {
private:
    std::vector<std::shared_ptr<Instruction>> instructions;
    int repeats;
    mutable int currentIteration;
    mutable int currentInstructionIndex;

public:
    ForInstruction(const std::vector<std::shared_ptr<Instruction>>& instrs, int reps)
        : instructions(instrs), repeats(reps), currentIteration(0), currentInstructionIndex(0), type(FOR) {
    }

    bool execute(ProcessContext& context) override {
        if (currentIteration >= repeats) {
            return true;  // For loop completed
        }

        if (currentInstructionIndex >= instructions.size()) {
            // Finished current iteration
            currentIteration++;
            currentInstructionIndex = 0;

            if (currentIteration >= repeats) {
                return true;  // All iterations complete
            }
        }

        // Execute current instruction
        if (currentInstructionIndex < instructions.size()) {
            bool instructionComplete = instructions[currentInstructionIndex]->execute(context);
            if (instructionComplete) {
                currentInstructionIndex++;
            }
        }

        return false;  // For loop not yet complete
    }

    std::string toString() const override {
        return "FOR([" + std::to_string(instructions.size()) + " instructions], " + std::to_string(repeats) + ")";
    }
};

class WriteInstruction : public Instruction {
private:
    uint16_t address;
    std::string variable;

public:
    WriteInstruction(uint16_t addr, const std::string& var)
        : address(addr), variable(var), type(WRITE) {
    }

    bool execute(ProcessContext& context) override {
        uint16_t value = context.getVariable(variable);
        context.writeMemory(address, value);
        return true;
    }

    std::string toString() const override {
        std::stringstream ss;
        ss << "WRITE(0x" << std::hex << address << ", " << variable << ")";
        return ss.str();
    }

    bool memoryAccessed() const override { 
        return true; 
    }

    std::optional<uint16_t> getMemoryAddress() const override {
        return address;
    }
};


class ReadInstruction : public Instruction {
private:
    std::string variable;
    uint16_t address;

public:
    ReadInstruction(const std::string& var, uint16_t addr)
        : variable(var), address(addr), type(READ) {
    }

    bool execute(ProcessContext& context) override {
        uint16_t value = context.readMemory(address);
        context.setVariable(variable, value);
        return true;
    }

    std::string toString() const override {
        std::stringstream ss;
        ss << "READ(" << variable << ", 0x" << std::hex << address << ")";
        return ss.str();
    }

    bool memoryAccessed() const override {
        return true;
    }

    std::optional<uint16_t> getMemoryAddress() const override {
        return address;
    }
};

#endif // INSTRUCTION_H