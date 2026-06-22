#pragma once
#include <vector>
#include <mutex>

template<typename T>
class DoubleBuffer
{
private:
	std::vector<T> bufferA;
	std::vector<T> bufferB;
	std::vector<T>* back;
	std::mutex lock;

public:
	DoubleBuffer() : back(&bufferA) {}

	//Back 버퍼에 삽입
	void Push(T item) {
		lock.lock();
		back->push_back(std::move(item));
		lock.unlock();
	}

	// Front 버퍼와 Back 버퍼 스왑
	void Swap(std::vector<T>& out) {
		lock.lock();
		std::swap(*back, out);
		lock.unlock();
	}
};