#pragma once
#include "memoryHandler.h"
// ------------------------------------------------------------
// FixedVector class (fixed-capacity, no heap)
// ------------------------------------------------------------
template <typename T>
class FixedVector
{
public:
	FixedVector()
		: data_(nullptr), size_(0), capacity_(0)
	{}

	bool init(uint24_t max_elements)
	{
		MemHandler &mem = MemHandler::getInstance();
		mem.validateMemIntegrity();
		const uint24_t iFreeBytes = mem.getFreeMemoryBytes();

		const uint24_t required = max_elements * sizeof(T);

		void *pFreeMem = mem.permaAllocMemory(required);
		mem.validateMemIntegrity();

		if (pFreeMem == nullptr) {
			dbg_sprintf(dbgout, "\nFree User RAM: %zu at %p for %d elements", iFreeBytes, pFreeMem, max_elements);

			return false;
		}

		data_ = static_cast<T *>(pFreeMem);
		size_ = 0;
		capacity_ = max_elements;

		dbg_sprintf(dbgout, "\nFree User RAM: %zu at %p for %d elements", iFreeBytes, pFreeMem, max_elements);


		return true;
	}

	// Capacity
	uint24_t size() const { return size_; }
	uint24_t capacity() const { return capacity_; }
	bool empty() const { return size_ == 0; }
	bool full() const { return size_ == capacity_; }

	// Modifiers
	bool push_back(const T &value)
	{
		if (size_ >= capacity_)
			return false;

		data_[size_++] = value;
		return true;
	}

	bool pop_back()
	{
		if (size_ == 0)
			return false;

		size_--;
		return true;
	}

	void clear()
	{
		size_ = 0;
	}

	// --------------------------------------------------------
	// Insert / Erase
	// --------------------------------------------------------
	bool insert(uint24_t index, const T &value)
	{
		if (size_ >= capacity_)
			return false;

		if (index > size_)
			return false;

		// Shift right
		for (uint24_t i = size_; i > index; --i) {
			data_[i] = data_[i - 1];
		}

		data_[index] = value;
		size_++;
		return true;
	}

	bool removeAt(uint24_t index)
	{
		if (index >= size_)
			return false;

		// Shift left
		for (uint24_t i = index; i + 1 < size_; ++i) {
			data_[i] = data_[i + 1];
		}

		size_--;
		return true;
	}

	// Element access
	T &operator[](uint24_t index)
	{
		return data_[index];
	}

	const T &operator[](uint24_t index) const
	{
		return data_[index];
	}

	T *data() { return data_; }
	const T *data() const { return data_; }

private:
	T *data_;
	uint24_t size_;
	uint24_t capacity_;

	static void *align_ptr(void *p, uint24_t align)
	{
		uintptr_t x = reinterpret_cast<uintptr_t>(p);
		x = (x + align - 1) & ~(align - 1);
		return reinterpret_cast<void *>(x);
	}
};