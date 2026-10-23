#pragma once

#include <filesystem>
#include <vector>

#include "taskmgr/Task.h"

namespace taskmgr {

// Abstract storage backend. TaskManager depends only on this interface,
// so it can be tested without touching the file system.
class IStorage {
public:
    virtual ~IStorage() = default;
    virtual std::vector<Task> load() = 0;
    virtual void save(const std::vector<Task>& tasks) = 0;
};

// Keeps tasks in memory only. Used in tests.
class MemoryStorage : public IStorage {
public:
    std::vector<Task> load() override { return data_; }
    void save(const std::vector<Task>& tasks) override { data_ = tasks; }
    const std::vector<Task>& data() const { return data_; }

private:
    std::vector<Task> data_;
};

// Stores tasks in a tab-separated text file:
//   id <TAB> done(0/1) <TAB> priority <TAB> due or "-" <TAB> escaped title
class FileStorage : public IStorage {
public:
    explicit FileStorage(std::filesystem::path path);

    // A missing file is not an error: it simply means "no tasks yet".
    std::vector<Task> load() override;

    // Writes to a temporary file first, then replaces the real one,
    // so a crash in the middle of saving cannot corrupt existing data.
    void save(const std::vector<Task>& tasks) override;

private:
    std::filesystem::path path_;
};

}  // namespace taskmgr
