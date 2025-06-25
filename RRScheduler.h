#pragma once

#include <iostream> // debuug

#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <chrono>
#include "Process.h"
#include "CPUWorker.h"
#include "Scheduler.h"

using namespace std;
using namespace chrono;
using namespace this_thread;

class RRScheduler: public Scheduler {
private:
	const int cores;                                  // number of CPUs, constant -> fixed by default
	const int quantum;                                // time quantum, also fixed
	queue<shared_ptr<Process>> readyQueue;            // ready queue (uses shared pointers for Process objects)
	//new
	std::vector<std::shared_ptr<Process>> processes; // ensure you track all procs here
	vector<CPU> cpus;                                 // container of CPU objects

	atomic<bool> endThreads;                          // marker for ending threads (atomic, so it's thread-safe)
	vector<thread> cpuThreads;                        // container of CPU threads
	mutex schedulerMutex;                             // manage shared access to the ready queue
	mutable mutex cpuMutex;                           // for lockinbg CPUs
	mutex logMutex;                                   // locking logs 
	condition_variable queueCondition;                // used for telling CPU threads when a process is available in ready queue
	bool verbose = false;


public:
	RRScheduler(int cores, int quantum);

	CPU* getCPU();

	void initializeCPUs(int numCPUs);

	void enqueueProcess(shared_ptr<Process> process);

	void scheduleCPU();

	void start();

	void stop();
	void setVerbose(bool v) { verbose = v; }
	bool allProcessesFinished() const;
	void executeOneStep();


	//new
	std::shared_ptr<Process> getProcess(const std::string& name) const override;

};