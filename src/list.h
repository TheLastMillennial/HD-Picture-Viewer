#pragma once

/* A limited list implementation */

#include <debug.h>
// Define a Node structure
template <typename T>
struct Node {
    T data;
    Node* next;
    Node* prev;

    // Constructor (C++98 compatible initialization)
    Node(const T& data) {
        this->data = data;
        this->next = NULL;
        this->prev = NULL;
    }
};

// Define the List class
template <typename T>
class List {
private:
    Node<T>* head;
    Node<T>* tail;
    size_t size;

public:
    // Constructor
    List() {
        head = NULL;
        tail = NULL;
        size = 0;
    }

    // Destructor to free memory
    ~List() {
        clear();
    }

    Node<T> get(uint24_t index)
    {
        Node<T>* head
        for (int i{ 0 }; i < index;i++)
        {

        }
    }

    // Push data to the front of the list
    void push_front(const T& data) {
        Node<T>* new_node = new Node<T>(data);
        if (head == NULL) {
            head = tail = new_node;  // First node in the list
        }
        else {
            new_node->next = head;
            head->prev = new_node;
            head = new_node;
        }
        ++size;
    }

    // Push data to the back of the list
    void push_back(const T& data) {
        Node<T>* new_node = new Node<T>(data);
        if (tail == NULL) {
            head = tail = new_node;  // First node in the list
        }
        else {
            new_node->prev = tail;
            tail->next = new_node;
            tail = new_node;
        }
        ++size;
    }

    // Pop data from the front of the list
    void pop_front() {
        if (head == NULL) {
            dbg_sprintf(dbgout, "List is empty");
            return;
        }
        Node<T>* temp = head;
        head = head->next;
        if (head != NULL) {
            head->prev = NULL;
        }
        else {
            tail = NULL;  // List is now empty
        }
        delete temp;
        --size;
    }

    // Pop data from the back of the list
    void pop_back() {
        if (tail == NULL) {
            dbg_sprintf(dbgout, "List is empty");
            return;
        }
        Node<T>* temp = tail;
        tail = tail->prev;
        if (tail != NULL) {
            tail->next = NULL;
        }
        else {
            head = NULL;  // List is now empty
        }
        delete temp;
        --size;
    }

    // Insert data at a specific position
    void insert(size_t index, const T& data) {
        if (index > size) {
            dbg_sprintf(dbgout, "Index out of range");
            return;
        }

        if (index == 0) {
            push_front(data);
        }
        else if (index == size) {
            push_back(data);
        }
        else {
            Node<T>* new_node = new Node<T>(data);
            Node<T>* current = head;
            for (size_t i = 0; i < index; ++i) {
                current = current->next;
            }
            new_node->prev = current->prev;
            new_node->next = current;
            if (current->prev != NULL) {
                current->prev->next = new_node;
            }
            current->prev = new_node;
            ++size;
        }
    }

    // Erase data from a specific position
    void erase(size_t index) {
        if (index >= size) {
            dbg_sprintf(dbgout, "Index out of range");
            return;
        }

        if (index == 0) {
            pop_front();
        }
        else if (index == size - 1) {
            pop_back();
        }
        else {
            Node<T>* current = head;
            for (size_t i = 0; i < index; ++i) {
                current = current->next;
            }
            current->prev->next = current->next;
            if (current->next != NULL) {
                current->next->prev = current->prev;
            }
            delete current;
            --size;
        }
    }

    // Clear the entire list
    void clear() {
        while (head != NULL) {
            pop_front();
        }
    }

    // Size of the list
    size_t size() const {
        return size;
    }

    // Check if the list is empty
    bool empty() const {
        return size == 0;
    }
};


