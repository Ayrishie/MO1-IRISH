#pragma once

#include <iostream>
#include <unordered_map>
#include <queue>
#include <list>
#include <vector>
#include <memory>
#include "Process.h"

// MEMORYALLOCATOR: INTERFACE
class MemoryAllocator {
public:
	virtual void allocate(std::shared_ptr<Process> process, size_t pageNumber) = 0;
	virtual void deallocate(std::shared_ptr<Process> process) = 0;
	virtual void visualizeMemory() const = 0;
};

// Memory Allocator Implementation for Demand Paging Algorithm
class PagingAllocator : public MemoryAllocator{
private:
	size_t maxMemorySize;
	size_t numFrames;
	size_t memoryPerFrame;
	std::unordered_map<size_t, int> frameMap; // frame index-process id pair

	std::queue<size_t> freeFrameList; // for tracking free frames
	std::list<size_t> accessedFrames; // for LRU page replacement (tracks frames in order of access -> front = LRU)

	//std::vector<size_t> allocateFrames(size_t numFrames, int process_id);
	//void deallocateFrames(const std::vector<size_t>& frameIndices);

public:
	PagingAllocator(size_t maxMemorySize, size_t memPerFrame);
	~PagingAllocator() = default;

	void allocate(std::shared_ptr<Process> process, size_t pageNumber) override;
	void deallocate(std::shared_ptr<Process> process) override;
	void visualizeMemory() const override;

	void evictFrame();
};

