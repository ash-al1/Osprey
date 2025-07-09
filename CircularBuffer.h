#pragma once

#include <vector>
#include <mutex>

template<typename T>
class CircularBuffer{
private:
	std::vector<T> data_;
	size_t head_;	  // Current pos
	size_t tail_;	  // Read pos
	size_t size_;	  // Number of elements
	size_t capacity_; // Max number of elements
	mutable std::mutex mutex_;

public:
	explicit CircularBuffer(size_t capacity)
        : data_(capacity)
        , head_(0)
        , tail_(0)
        , size_(0)
        , capacity_(capacity) {
    }

	// Push an element to the buffer
	void Push(const T& item) {
		std::lock_guard<std::mutex> lock(mutex_);
		data_[head_] = item;
		head_ = (head_ + 1) % capacity_;
		if (size_ < capacity_) { ++size_; }
		else { tail_ = (tail_ + 1) % capacity_; }
	}

	// Pop an element off the buffer
	bool Pop(T& item) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (size_ == 0) { return false; }
		item = data_[tail_];
		tail_ = (tail_ + 1) % capacity_;
		--size_;
		return true;
	}

	void CopyLatest(T* dest, size_t count) {
		std::lock_guard<std::mutex> lock(mutex_);
		size_t copy_count = std::min(count, size_);
		for ( size_t i = 0; i < copy_count; ++i ) {
			size_t idx = (tail_ + i) % capacity_;
			dest[i] = data_[idx];
		}
		for ( size_t i = copy_count; i < count; ++i ) {
			dest[i] = T{};
		}
	}

	// Get the current size of the buffer
	size_t Size() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return size_;
	}

	// Get the maximum number of allowable items in the buffer
	size_t Capacity() const {
		return capacity_;
	}

	// Check if buffer is empty
	bool IsEmpty() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return size_ == 0;
	}

	// Is buffer currently full
	bool IsFull() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return size_ == capacity_;
	}

	// Clear buffer when full
	void clear() const {
		std::lock_guard<std::mutex> lock(mutex_);
		head_ = tail_ = size_ = 0;
	}
};

