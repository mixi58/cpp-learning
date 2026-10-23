#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "taskmgr/Storage.h"
#include "taskmgr/Task.h"

namespace taskmgr {

struct TaskFilter {
    std::optional<bool> done;
    std::optional<Priority> priority;
};

class TaskManager {
public:
    explicit TaskManager(std::unique_ptr<IStorage> storage);

    Task add(std::string title, Priority priority = Priority::Medium,
             std::optional<std::string> due = std::nullopt);
    void remove(int id);
    void mark_done(int id);
    void edit_title(int id, std::string title);
    Task get(int id) const;

    // Not-done tasks first, then by priority (high to low), then by id.
    std::vector<Task> list(const TaskFilter& filter = {}) const;

    // Case-insensitive substring search in titles.
    std::vector<Task> search(const std::string& text) const;

    std::size_t size() const { return tasks_.size(); }

    void load();
    void save() const;

private:
    Task& find(int id);
    const Task& find(int id) const;

    std::vector<Task> tasks_;
    int next_id_{1};
    std::unique_ptr<IStorage> storage_;
};

}  // namespace taskmgr
