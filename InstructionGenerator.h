#pragma once
#ifndef INSTRUCTION_GENERATOR_H
#define INSTRUCTION_GENERATOR_H

#include "Instruction.h"
#include <random>
#include <vector>
#include <memory>
#include <algorithm> // std::max
#include <iostream>  // <-- add this


class InstructionGenerator {
private:
    // RNG + common distributions
    std::mt19937 rng;
    std::uniform_int_distribution<int> instructionTypeDist;     // 0..7
    std::uniform_int_distribution<uint16_t> valueDist;          // 0..1000
    std::uniform_int_distribution<int>   sleepDist;             // 1..10
    std::uniform_int_distribution<int>   repeatsDist;           // 1..5
    std::uniform_int_distribution<int>   forInstructionCountDist; // 1..3

    // Variable pool
    std::vector<std::string> variableNames{
        "x","y","z","a","b","c","counter","temp","result","value"
    };
    std::uniform_int_distribution<size_t> variableNameDist;     // 0..variableNames.size()-1

    // IMPORTANT: initialize memPerProc BEFORE any distribution that depends on it
    int memPerProc; // address space upper bound for random memory ops (bytes)

    // Member distribution for addresses (uses memPerProc). Bounds are clamped.
    std::uniform_int_distribution<int> addressDist;

public:
    // Optional: allow passing per-process memory size; defaults to 64 KiB safe range
    explicit InstructionGenerator(int memPerProcInit = 65536)
        : rng(std::random_device{}()),
        instructionTypeDist(0, 7),
        valueDist(0, 1000),
        sleepDist(1, 10),
        repeatsDist(1, 5),
        forInstructionCountDist(1, 3),
        variableNameDist(0, variableNames.empty() ? 0 : variableNames.size() - 1),
        // set memPerProc to a safe minimum of 2 so (hi = memPerProc-2) is >= 0
        memPerProc(std::max(2, memPerProcInit)),
        // clamp upper bound: if memPerProc == 2, hi becomes 0 => range [0,0]
        addressDist(0, std::max(0, memPerProc - 2))
    {
    }

    std::shared_ptr<Instruction> generateRandomInstruction(const std::string& processName,
        int nestingLevel = 0)
    {
        int type = instructionTypeDist(rng);

        // Limit nesting depth for FOR loops (max 3 levels as per spec)
        if (type == 5 && nestingLevel >= 3) {
            // Pick any type other than FOR
            type = instructionTypeDist(rng) % 5; // 0..4  (PRINT..SLEEP)
        }

        switch (type) {
        case 0: return generatePrintInstruction(processName);                 // PRINT
        case 1: return generateDeclareInstruction();                          // DECLARE
        case 2: return generateAddInstruction();                              // ADD
        case 3: return generateSubtractInstruction();                         // SUBTRACT
        case 4: return generateSleepInstruction();                            // SLEEP
        case 5: return generateForInstruction(processName, nestingLevel + 1); // FOR
        case 6: return generateReadInstruction();                             // READ
        case 7: return generateWriteInstruction();                            // WRITE
        default: return generatePrintInstruction(processName);
        }
    }

    std::vector<std::shared_ptr<Instruction>>
        generateInstructionSet(const std::string& processName, int count)
    {
        std::vector<std::shared_ptr<Instruction>> instructions;
        instructions.reserve(std::max(0, count));

        for (int i = 0; i < count; ++i) {
            instructions.push_back(generateRandomInstruction(processName));
        }
        return instructions;
    }

private:
    // ---- helpers -------------------------------------------------------------

    // Always returns an even, in-range 16-bit address.
    uint16_t pickAlignedAddr() {
        // Upper bound for distribution = memPerProc-2, clamped >= 0 in ctor
        int raw = addressDist(rng);
        // force alignment to 2 bytes
        return static_cast<uint16_t>((raw / 2) * 2);
    }

    std::string getRandomVariableName() {
        // variableNames is non-empty; distribution is 0..size-1
        return variableNames[variableNameDist(rng)];
    }

    // ---- instruction builders ------------------------------------------------

    std::shared_ptr<Instruction> generateAddInstruction() {
        std::string result = getRandomVariableName();
        std::string op1 = getRandomVariableName();
        std::string op2 = getRandomVariableName();

        // 20% chance to use literal values instead of variables
        bool op1IsLiteral = std::uniform_int_distribution<int>(0, 4)(rng) == 0;
        bool op2IsLiteral = std::uniform_int_distribution<int>(0, 4)(rng) == 0;

        if (op1IsLiteral && op2IsLiteral) {
            return std::make_shared<AddInstruction>(result, valueDist(rng), valueDist(rng));
        }
        else if (op1IsLiteral) {
            return std::make_shared<AddInstruction>(result, valueDist(rng), op2);
        }
        else if (op2IsLiteral) {
            return std::make_shared<AddInstruction>(result, op1, valueDist(rng));
        }
        else {
            return std::make_shared<AddInstruction>(result, op1, op2);
        }
    }

    std::shared_ptr<Instruction> generateSubtractInstruction() {
        std::string result = getRandomVariableName();
        std::string op1 = getRandomVariableName();
        std::string op2 = getRandomVariableName();

        bool op1IsLiteral = std::uniform_int_distribution<int>(0, 4)(rng) == 0;
        bool op2IsLiteral = std::uniform_int_distribution<int>(0, 4)(rng) == 0;

        if (op1IsLiteral && op2IsLiteral) {
            return std::make_shared<SubtractInstruction>(result, valueDist(rng), valueDist(rng));
        }
        else if (op1IsLiteral) {
            return std::make_shared<SubtractInstruction>(result, valueDist(rng), op2);
        }
        else if (op2IsLiteral) {
            return std::make_shared<SubtractInstruction>(result, op1, valueDist(rng));
        }
        else {
            return std::make_shared<SubtractInstruction>(result, op1, op2);
        }
    }

    std::shared_ptr<Instruction> generateSleepInstruction() {
        uint8_t cycles = static_cast<uint8_t>(sleepDist(rng));
        return std::make_shared<SleepInstruction>(cycles);
    }

    std::shared_ptr<Instruction> generateForInstruction(const std::string& processName,
        int nestingLevel)
    {
        int repeats = repeatsDist(rng);
        int instructionCount = forInstructionCountDist(rng);

        std::vector<std::shared_ptr<Instruction>> forInstructions;
        forInstructions.reserve(instructionCount);

        for (int i = 0; i < instructionCount; ++i) {
            forInstructions.push_back(generateRandomInstruction(processName, nestingLevel));
        }
        return std::make_shared<ForInstruction>(forInstructions, repeats);
    }

    std::shared_ptr<Instruction> generatePrintInstruction(const std::string& processName) {
        // Spec default
        std::string message = "Hello world from " + processName + "!";
        return std::make_shared<PrintInstruction>(message);
    }

    std::shared_ptr<Instruction> generateDeclareInstruction() {
        std::string varName = getRandomVariableName();
        uint16_t value = valueDist(rng);
        return std::make_shared<DeclareInstruction>(varName, value);
    }

    std::shared_ptr<Instruction> generateWriteInstruction() {
        std::string var = getRandomVariableName();
        uint16_t addr = pickAlignedAddr();
        return std::make_shared<WriteInstruction>(addr, var);
    }

    std::shared_ptr<Instruction> generateReadInstruction() {
        std::string var = getRandomVariableName();
        uint16_t addr = pickAlignedAddr();
        return std::make_shared<ReadInstruction>(var, addr);
    }
};

#endif // INSTRUCTION_GENERATOR_H
