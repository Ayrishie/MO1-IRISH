#include "RRScheduler.h"

RRScheduler::RRScheduler(int cores, int quantum, int delayPerExecution) 
    : cores(cores), quantum(quantum), delayPerExecution(delayPerExecution), scheduler_running(false) {}

RRScheduler::~RRScheduler() {
    stop();
}

void RRScheduler::scheduleCPU(int coreId) {
    while (scheduler_running) {
        shared_ptr<Process> process;
        { // lock the access for the ready queue
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [this] { //wait for aprocess or scheduler stop
                return !readyQueue.empty() || !scheduler_running;
                });

            if (!scheduler_running) break; // if scheduler stop then end the loop
            if (readyQueue.empty()) continue; // still nothing to run

            //dequeue next process
            process = readyQueue.front();
            readyQueue.pop();
        }

        if (!process || process->isFinished()) continue; // skip process if null or finished

        if (process && !process->isFinished()) {
            process->core_id = coreId;

            int quantumUsed = 0;
            while (quantumUsed < quantum && !process->isFinished()) {
                // get the amount of executed cinstruction first
                int prevInstructions = process->executed_commands;

                process->executeCommand(coreId); //execute the process

                if (process->executed_commands > prevInstructions) { //if instruction was executed then increment quantum cycle usage
                    quantumUsed++;
                }
                // Simulate work
                std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExecution));
            }

            process->core_id = -1;

            // requeue process if it isn't finished
            if (!process->isFinished()) {
                std::lock_guard<std::mutex> lock(queue_mutex);
                readyQueue.push(process);
                cv.notify_all();
            }
        }
    }
}

void RRScheduler::enqueueProcess(shared_ptr<Process> process) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        readyQueue.push(process);
        processes.push_back(process);
    }
    cv.notify_all();
}

void RRScheduler::start() {
    if (scheduler_running) return;

    scheduler_running = true;
    for (int i = 0; i < cores; ++i) {
        cpuThreads.emplace_back([this, i] { scheduleCPU(i); });
    }
}

// Join all threads to end the scheduler
void RRScheduler::stop() {
    scheduler_running = false;
    cv.notify_all();

    for (auto& thread : cpuThreads) {
        if (thread.joinable()) thread.join();
    }
    cpuThreads.clear();
}

void RRScheduler::displayProcesses() const {
    std::lock_guard<std::mutex> lock(queue_mutex);

    std::cout << "Active processes:\n";
    for (const auto& p : processes) {
        if (!p->isFinished()) {
            p->displayProcess();
        }
    }

    std::cout << "Completed processes:\n";
    for (const auto& p : processes) {
        if (p->isFinished()) {
            p->displayProcess();
        }
    }
}

void RRScheduler::displayProcesses(std::ostream& out) const {
    std::lock_guard<std::mutex> lock(queue_mutex);

    out << "Active processes:\n";
    for (const auto& p : processes) {
        if (!p->isFinished()) {
            p->displayProcess(out);  // this is the overloaded one
        }
    }

    out << "Completed processes:\n";
    for (const auto& p : processes) {
        if (p->isFinished()) {
            p->displayProcess(out);
        }
    }
}

bool RRScheduler::allProcessesFinished() const {
    std::lock_guard<std::mutex> lock(queue_mutex);
    for (const auto& p : processes) {
        if (!p->isFinished()) return false;
    }
    return true;
}

std::shared_ptr<Process> RRScheduler::getProcess(const std::string& name) const {
    for (auto& p : processes) {
        if (p->name == name) return p;
    }
    return nullptr;
}