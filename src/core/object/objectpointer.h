//
// Created by Vishnu Rajendran on 2024-09-18.
//
#pragma once

#ifndef METEOR_ENGINE_OBJECTPOINTER_H
#define METEOR_ENGINE_OBJECTPOINTER_H


#include "gc.h"

template<typename T>
class MObjectPtr {
    static_assert(std::is_base_of<MObject, T>::value, "T must derive from MObject");

private:
    T* ptr;

public:
    // Constructor
    MObjectPtr(T *obj = nullptr) : ptr(obj) {
        if (ptr != nullptr) {
            MGarbageCollector::reference(ptr);
        }
    }

    // Destructor
    ~MObjectPtr() {
        if (ptr != nullptr) {
            MGarbageCollector::dereference(ptr);
            reset();
        }
    }

    // Copy constructor
    MObjectPtr(const MObjectPtr &other) : ptr(other.ptr) {
        if (ptr != nullptr) {
            MGarbageCollector::reference(ptr);
        }
    }

    // Move constructor
    MObjectPtr(MObjectPtr &&other) noexcept: ptr(other.ptr) {
        other.ptr = nullptr;  // Ensure the source pointer is nullified
    }

    // Copy assignment operator
    MObjectPtr &operator=(const MObjectPtr &other) {
        if (this == &other) {
            return *this;  // Handle self-assignment
        }

        // Dereference the current object
        if (ptr != nullptr) {
            MGarbageCollector::dereference(ptr);
        }

        ptr = other.ptr;

        // Reference the new object
        if (ptr != nullptr) {
            MGarbageCollector::reference(ptr);
        }

        return *this;
    }

    // Move assignment operator
    MObjectPtr &operator=(MObjectPtr &&other) noexcept {
        if (this != &other) {
            if (ptr != nullptr) {
                MGarbageCollector::dereference(ptr);
            }

            ptr = other.ptr;
            other.ptr = nullptr;  // Nullify the source pointer
        }

        return *this;
    }

    // Assign nullptr
    MObjectPtr &operator=(std::nullptr_t) {
        if (ptr != nullptr) {
            MGarbageCollector::dereference(ptr);
            reset();
            ptr = nullptr;
        }
        return *this;
    }

    // Overload dereference operator
    T &operator*() const {
        return *ptr;
    }

    // Overload arrow operator
    T *operator->() const {
        return ptr;
    }

    // Support comparison with nullptr
    bool operator==(std::nullptr_t) const {
        return ptr == nullptr;
    }

    bool operator!=(std::nullptr_t) const {
        return ptr != nullptr;
    }

    void reset(){
        ptr = nullptr;
    }
};

#endif //METEOR_ENGINE_OBJECTPOINTER_H
