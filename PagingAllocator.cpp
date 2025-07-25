#include "MemoryAllocator.h"

/*
	CONSTRUCTOR METHOD

	numFrames of physical memory = max memory size / memory size per frame
*/
PagingAllocator::PagingAllocator(size_t maxMemorySize, size_t memPerFrame) : 
	maxMemorySize(maxMemorySize), numFrames((maxMemorySize + memPerFrame - 1 ) / memPerFrame) {
	
	// Initialize free frames (all frames are initially free)
	for (size_t i = 0; i < numFrames; ++i) {
		freeFrameList.push_back(i);
	}
}

std::vector<size_t> PagingAllocator::allocateFrames(size_t numFrames, size_t processId) {
	std::vector<size_t> allocatedFrames;

	if (freeFrameList.size() < numFrames) {
		return allocatedFrames; // this returns an empty vector = Fail to allocate frames
	}

	for (size_t i = 0; i < numFrames; ++i) {
		size_t frame = freeFrameList.back();
		freeFrameList.pop_back();
		frameMap[frame] = processId;
		allocatedFrames.push_back(frame);
	}

	return allocatedFrames;
}

void PagingAllocator::deallocateFrames(const std::vector<size_t>& frameIndices) {
	for (size_t frameIndex : frameIndices) {
		frameMap.erase(frameIndex);
		freeFrameList.push_back(frameIndex);
	}
}

void PagingAllocator::deallocate(Process* process) {
	size_t processId = process->process_id;
	std::vector<size_t> framesToDeallocate;

	// Collect all frames owned by the process
	for (const auto& [frame, pid] : frameMap) {
		if (pid == processId) {
			framesToDeallocate.push_back(frame);
		}
	}

	deallocateFrames(framesToDeallocate);

	// reset the page table to init
	for (auto& page : process->pageTable) {
		page.frame_number = -1;
		page.inMemory = false;
		page.swapped = false;
	}
}

void PagingAllocator::visualizeMemory() const {
	for (size_t frameIndex = 0; frameIndex < numFrames; ++frameIndex) {
		auto it = frameMap.find(frameIndex);
		if (it != frameMap.end()) {
			std::cout << "Frame " << frameIndex << " -> Process " << it->second << "\n";
		}
		else {
			std::cout << "Frame " << frameIndex << " -> Free\n";
		}
	}
	std::cout << "---------------------------\n";
}