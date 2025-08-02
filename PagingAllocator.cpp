#include "MemoryAllocator.h"

/*
	CONSTRUCTOR METHOD

	numFrames of physical memory = max memory size / memory size per frame
*/
PagingAllocator::PagingAllocator(size_t maxMemorySize, size_t memoryPerFrame) : 
	maxMemorySize(maxMemorySize), 
	memoryPerFrame(memoryPerFrame),
	numFrames((maxMemorySize + memoryPerFrame - 1 ) / memoryPerFrame) {
	// Initialize free frames (all frames are initially free)
	for (size_t i = 0; i < numFrames; ++i) {
		freeFrameList.push(i);
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

void PagingAllocator::allocate(std::shared_ptr<Process> process, size_t pageNumber) {
	auto& page = process->page_table[pageNumber];

	if (page.inMemory) {
		if (freeFrameList.empty()) {
			evictFrame();
		}

		size_t frame = freeFrameList.front();
		freeFrameList.pop();

		frameMap[frame] = process->process_id;
		accessedFrames.push_back(frame);

		page.frame_number = frame;
		page.inMemory = true;
		page.swapped = false;
	}
}

// USED FOR EVICTING ALL PROCESS PAGES (PROCESS-SPECIFIC)
void PagingAllocator::deallocate(std::shared_ptr<Process> process) {
	int processId = process->process_id;
	std::vector<size_t> framesToDeallocate;
	
	// Collect all frames owned by the process
	for (const auto& [frame, pid] : frameMap) {
		if (pid == processId) {
			framesToDeallocate.push_back(frame);
		}
	}
	
	for (size_t frameIndex : framesToDeallocate) {
		frameMap.erase(frameIndex);
		freeFrameList.push(frameIndex);
	}
	
	// reset the page table to init
	process->page_table.clear();
}

// USED FOR EVICTING PAGES (MEMORY-WIDE); USES LRU
void PagingAllocator::evictFrame() {
	if (accessedFrames.empty()) return;

	// identify the Least Recently Used frame
	size_t frame = accessedFrames.front();
	accessedFrames.pop_front();

	//remove from frameMap and make the frame free again
	frameMap.erase(frame);
	freeFrameList.push(frame);
}