#pragma once
#include <queue>
#include <mutex>

template<typename T>
class EventQueue {
private:
	std::queue<T> m_queue;
	std::mutex m_mutex;

public:
	void Push(T item) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push(item);
	}

	bool Pop(T& outItem) {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_queue.empty()) return false;
		outItem = std::move(m_queue.front());
		m_queue.pop();
		return true;
	}
};