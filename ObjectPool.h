#pragma once
#include <vector>
#include <cstdlib>

template <typename T>
class ObjectPool {
public:
	ObjectPool(size_t initialSize = 100) {
		Expand(initialSize);
	}

	~ObjectPool() {
		for (void* block : mAllBlocks) {
			std::free(block);
		}
	}

	T* Allocate() {
		if (mFreeList.empty()) {
			Expand(mGrowSize);
		}
		T* ptr = mFreeList.back();
		mFreeList.pop_back();
		return ptr;
	}

	void Free(T* ptr) {
		mFreeList.push_back(ptr);
	}

private:
	// 増設
	void Expand(size_t size) {
		T* newBlock = static_cast<T*>(std::malloc(sizeof(T) * size));
		for (size_t i = 0; i < size; ++i) {
			mFreeList.push_back(&newBlock[i]);
		}
		mAllBlocks.push_back(newBlock);
	}

	std::vector<T*> mFreeList;
	std::vector<void*> mAllBlocks; 
	size_t mGrowSize = 64;
};