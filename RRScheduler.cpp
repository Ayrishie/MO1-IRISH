#include "RRScheduler.h"

// Constructor
RRScheduler::RRScheduler(int cores, int quantum) : cores(cores), quantum(quantum), endThreads(false) {
    for (int i = 0; i < cores; i++) {
        this->cpus.push_back(CPU(i)); // see CPU class constructor
    }
}

// Create a number of CPUs
void RRScheduler::initializeCPUs(int numCPUs) {
    for (int i = 0; i < numCPUs; i++) {
        this->cpus.push_back(CPU(i)); // see CPU class constructor
    }
}

// Find a free CPU sequentially (top-bottom )  
CPU* RRScheduler::getCPU() {
    lock_guard<mutex> lock(cpuMutex); // protect the check and update atomically
    for (auto& cpu : this->cpus) {
        if (cpu.getStatus() == IDLE) {
            cpu.setStatus(BUSY); // claim it immediately
            return &cpu;
        }
    }
    return nullptr;
}

// Method for the ready queue (enqueues processes at the tail of the ready queue) 
void RRScheduler::enqueueProcess(shared_ptr<Process> process) {
    lock_guard<mutex> lock(schedulerMutex);   // make sure access is restricted to one thread at a time
    this->readyQueue.push(process);           // only then enqueue a process at the ready queue
    queueCondition.notify_one();                  // tell a waiting thread that a process is now available (see scheduleCPU() for more)
}

// Method for a CPU thread (this is what a specific CPU does)
void RRScheduler::scheduleCPU() {
    while (!endThreads) {
        shared_ptr<Process> process;
        CPU* cpu = getCPU();

        if (!cpu) {
            this_thread::sleep_for(chrono::milliseconds(10));
            continue;
        }

        {
            unique_lock<mutex> lock(schedulerMutex);
            if (readyQueue.empty()) {
                cpu->setStatus(IDLE);
                lock.unlock();
                this_thread::sleep_for(chrono::milliseconds(10));
                continue;
            }

            process = readyQueue.front();
            readyQueue.pop();
        }

        int timeUsed = 0;
        while (timeUsed < quantum && !process->isFinished()) {
            cpu->runProcess(process);
            timeUsed++;
            this_thread::sleep_for(chrono::milliseconds(50)); // Simulate work
        }

        if (!process->isFinished()) {
            enqueueProcess(process);
        }
        cpu->setStatus(IDLE);
    }
}

// Main entry point for the round robin scheduler (this is where all threads start, and end)
void RRScheduler::start() {
    endThreads = false;
    cpuThreads.clear();
    for (int i = 0; i < this->cores; i++) {
        cpuThreads.emplace_back([this]() { this->scheduleCPU(); });
    }
}

// Join all threads to end the scheduler
void RRScheduler::stop() {
    // End the scheduler
    {
        lock_guard<mutex> lock(schedulerMutex);            // lock again during shutting down
        endThreads = true;                                // time for joining threads
    }
    queueCondition.notify_all();               //tell all threads for exit

    // join threads
    for (auto& thread : cpuThreads) {
        if (thread.joinable()) {
            thread.join();                       
        }
    }
}

std::shared_ptr<Process> RRScheduler::getProcess(const std::string& name) const {
    for (auto& p : processes) {
        if (p->name == name) return p;
    }
    return nullptr;
}