// FCFSScheduler.cpp
#include "FCFSScheduler.h"
#include "MemoryAllocator.h"    // for PagingAllocator
#include "Process.h"
#include <iostream>
#include <chrono>
#include <thread>

FCFSScheduler::FCFSScheduler(int cores,
    int delayPerExecution,
    PagingAllocator* pagingAllocator)
    : cores(cores)
    , delayPerExecution(delayPerExecution)
    , scheduler_running(false)
    , pagingAllocator(pagingAllocator)
{
}

FCFSScheduler::~FCFSScheduler() {
    stop();
}

void FCFSScheduler::start() {
    if (scheduler_running) return;
    scheduler_running = true;
    for (int i = 0; i < cores; ++i) {
        cpu_threads.emplace_back([this, i] { cpuWorker(i); });
    }
}

void FCFSScheduler::stop() {
    scheduler_running = false;
    cv.notify_all();
    for (auto& t : cpu_threads) {
        if (t.joinable()) t.join();
    }
    cpu_threads.clear();
}

void FCFSScheduler::addProcess(std::shared_ptr<Process> process) {
    // push into the ready queue and pre-allocate its first page
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (!pagingAllocator) return;               // no allocator → bail
        pagingAllocator->allocate(process, 0);      // grab page 0
        ready_queue.push(process);
    }
    processes.push_back(process);
    cv.notify_one();
}

void FCFSScheduler::cpuWorker(int coreId) {
    while (scheduler_running) {
        std::shared_ptr<Process> proc;
        {   // wait for a job
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [this] {
                return !ready_queue.empty() || !scheduler_running;
                });
            if (!scheduler_running) break;
            proc = ready_queue.front();
            ready_queue.pop();
        }

        if (!proc || proc->isFinished()) continue;

        // Run to completion
        proc->core_id = coreId;
        while (!proc->isFinished()) {
            proc->executeCommand(coreId);
            std::this_thread::sleep_for(
                std::chrono::milliseconds(delayPerExecution)
            );
        }
        proc->core_id = -1;

        // free _all_ pages belonging to this process
        if (pagingAllocator) {
            pagingAllocator->deallocate(proc);
        }
    }
}

void FCFSScheduler::displayProcesses() const {
    std::lock_guard<std::mutex> lock(queue_mutex);
    std::cout << "Active processes:\n";
    for (auto& p : processes)
        if (!p->isFinished()) p->displayProcess();
    std::cout << "Completed processes:\n";
    for (auto& p : processes)
        if (p->isFinished()) p->displayProcess();
}

void FCFSScheduler::displayProcesses(std::ostream& out) const {
    std::lock_guard<std::mutex> lock(queue_mutex);
    out << "Active processes:\n";
    for (auto& p : processes)
        if (!p->isFinished()) p->displayProcess(out);
    out << "Completed processes:\n";
    for (auto& p : processes)
        if (p->isFinished()) p->displayProcess(out);
}

bool FCFSScheduler::allProcessesFinished() const {
    std::lock_guard<std::mutex> lock(queue_mutex);
    for (auto& p : processes)
        if (!p->isFinished()) return false;
    return true;
}

std::shared_ptr<Process> FCFSScheduler::getProcess(const std::string& name) const {
    for (auto& p : processes)
        if (p->getName() == name) return p;
    return nullptr;
}
