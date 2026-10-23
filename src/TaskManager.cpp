#include "taskmgr/TaskManager.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <stdexcept>
#include <utility>

#include "taskmgr/Errors.h"

namespace taskmgr {
namespace {

std::string trim(const std::string& text) {
    const auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    auto first = std::find_if_not(text.begin(), text.end(), is_space);
    auto last = std::find_if_not(text.rbegin(), text.rend(), is_space).base();
    return first < last ? std::string(first, last) : std::string();
}

std::string to_lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

}  // namespace

TaskManager::TaskManager(std::unique_ptr<IStorage> storage) : storage_(std::move(storage)) {
    if (!storage_) throw std::invalid_argument("storage must not be null");
}

Task TaskManager::add(std::string title, Priority priority, std::optional<std::string> due) {
    title = trim(title);
    if (title.empty()) throw TaskError("title must not be empty");
    if (due && !is_valid_date(*due)) {
        throw TaskError("invalid date (expected YYYY-MM-DD): " + *due);
    }

    Task task;
    task.id = next_id_++;
    task.title = std::move(title);
    task.priority = priority;
    task.due = std::move(due);
    tasks_.push_back(task);
    return task;
}

void TaskManager::remove(int id) {
    const auto it = std::find_if(tasks_.begin(), tasks_.end(),
                                 [id](const Task& t) { return t.id == id; });
    if (it == tasks_.end()) throw TaskError("task not found: " + std::to_string(id));
    tasks_.erase(it);
}

void TaskManager::mark_done(int id) { find(id).done = true; }

void TaskManager::edit_title(int id, std::string title) {
    title = trim(title);
    if (title.empty()) throw TaskError("title must not be empty");
    find(id).title = std::move(title);
}

Task TaskManager::get(int id) const { return find(id); }

std::vector<Task> TaskManager::list(const TaskFilter& filter) const {
    std::vector<Task> result;
    std::copy_if(tasks_.begin(), tasks_.end(), std::back_inserter(result), [&](const Task& t) {
        if (filter.done && t.done != *filter.done) return false;
        if (filter.priority && t.priority != *filter.priority) return false;
        return true;
    });

    std::stable_sort(result.begin(), result.end(), [](const Task& a, const Task& b) {
        if (a.done != b.done) return !a.done;
        if (a.priority != b.priority) return a.priority > b.priority;
        return a.id < b.id;
    });
    return result;
}

std::vector<Task> TaskManager::search(const std::string& text) const {
    const std::string needle = to_lower(text);
    std::vector<Task> result;
    std::copy_if(tasks_.begin(), tasks_.end(), std::back_inserter(result), [&](const Task& t) {
        return to_lower(t.title).find(needle) != std::string::npos;
    });
    return result;
}

void TaskManager::load() {
    tasks_ = storage_->load();
    int max_id = 0;
    for (const Task& t : tasks_) max_id = std::max(max_id, t.id);
    next_id_ = max_id + 1;
}

void TaskManager::save() const { storage_->save(tasks_); }

Task& TaskManager::find(int id) {
    const auto it = std::find_if(tasks_.begin(), tasks_.end(),
                                 [id](const Task& t) { return t.id == id; });
    if (it == tasks_.end()) throw TaskError("task not found: " + std::to_string(id));
    return *it;
}

const Task& TaskManager::find(int id) const {
    const auto it = std::find_if(tasks_.begin(), tasks_.end(),
                                 [id](const Task& t) { return t.id == id; });
    if (it == tasks_.end()) throw TaskError("task not found: " + std::to_string(id));
    return *it;
}

}  // namespace taskmgr
