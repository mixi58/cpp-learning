#pragma once

#include <stdexcept>

namespace taskmgr {

// Thrown when the user asks for something invalid (unknown id, empty title, ...).
class TaskError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Thrown when tasks cannot be read from or written to storage.
class StorageError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

}  // namespace taskmgr
