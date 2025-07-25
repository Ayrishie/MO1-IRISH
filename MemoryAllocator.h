#pragma once

#include <unordered_map>
#include <vector>
#include "Process.h"

// MEMORYALLOCATOR: INTERFACE
class MemoryAllocator {
public:
	//virtual void* allocate(Process* process) = 0;
	virtual void deallocate(Process* process) = 0;
	virtual void visualizeMemory() const = 0;
};

// Memory Allocator Implementation for Demand Paging Algorithm
class PagingAllocator : public MemoryAllocator{
private:
	size_t maxMemorySize;
	size_t numFrames;
	std::unordered_map<size_t, size_t> frameMap; // frame index-process id pair
	std::vector<size_t> freeFrameList;

	std::vector<size_t> allocateFrames(size_t numFrames, size_t process_id);
	void deallocateFrames(size_t numFrames, size_t frameIndex);

public:
	PagingAllocator(size_t maxMemorySize, size_t memPerFrame);

	void deallocate(Process* process) override;
	void visualizeMemory() override;
};