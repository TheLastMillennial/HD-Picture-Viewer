#pragma once
/* A limited vector implementation */

template <typename T>
class Vector {
private:
    T* data;            // Pointer to the array holding the data
    uint24_t capacity;    // Capacity of the vector (maximum number of elements it can hold)
    uint24_t size;        // Current size (number of elements in the vector)

public:
    // Default constructor
    Vector() : data(NULL), capacity(0), size(0) {}

    // Destructor to free the dynamically allocated memory
    ~Vector() {
        delete[] data;
    }

    // Function to return the number of elements in the vector
    uint24_t getSize() const {
        return size;
    }

    // Function to return the current capacity of the vector
    uint24_t getCapacity() const {
        return capacity;
    }

    // Function to check if the vector is empty
    bool isEmpty() const {
        return size == 0;
    }

    // Function to add an element to the vector
    void push_back(const T& value) {
        if (size == capacity) {
            // If the vector is full, resize it (double the capacity)
            resize(capacity == 0 ? 1 : capacity * 2);
        }
        data[size++] = value;
    }

    // Function to get an element at a specific index
    T& operator[](uint24_t index) {
        if (index >= size) {
            dbg_sprintf(dbgout, "\nIndex out of range.\n Index: %d\n Vector Size: %d\n",index,size);
            return data[0];
        }
        return data[index];
    }

    // Function to get a const element at a specific index (const version)
    const T& operator[](uint24_t index) const {
        if (index >= size) {
            dbg_sprintf(dbgout, "\nIndex out of range.\n Index: %d\n Vector Size: %d\n", index, size);
            return data[0];
        }
        return data[index];
    }

    // Function to remove the last element
    void pop_back() {
        if (size > 0) {
            --size;
        }
        else {
            dbg_sprintf(dbgout, "Cannot pop from empty vector");
        }
    }

    // Function to reserve capacity for a specific number of elements
    void reserve(uint24_t new_capacity) {
        if (new_capacity > capacity) {
            resize(new_capacity);
        }
    }

    // Function to remove an element from the vector at a specific index
    void removeAt(uint24_t index) {
        if (index >= size) {
            dbg_sprintf(dbgout, "\nIndex out of range.\n Index: %d\n Vector Size: %d\n", index, size);
            return;
        }

        // Shift all elements after the specified index to the left by one position
        for (size_t i = index; i < size - 1; ++i) {
            data[i] = data[i + 1];
        }

        // Decrease the size
        --size;

        //This is slower, but frees up precious memory.
        // HD Pic only adds elements at startup so resizing smaller can make sense.
        resize(size);
    }

    // Function to clear all elements from the vector
    void clear() {
        size = 0;
        
        //delete[] data;  // Free the memory
        //data = NULL;    // Reset the data pointer to NULL
        //capacity = 0;   // Reset capacity
    }

  

private:
    // Private function to resize the vector to a new capacity
    void resize(uint24_t new_capacity) {
        T* new_data = new T[new_capacity];  // Allocate new memory

        // Copy old data to the new memory
        for (uint24_t i = 0; i < size; ++i) {
            new_data[i] = data[i];
        }

        // Free the old memory
        delete[] data;

        // Update the vector's data and capacity
        data = new_data;
        capacity = new_capacity;
    }
};

