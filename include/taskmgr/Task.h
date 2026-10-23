#pragma once

#include <optional>
#include <string>

namespace taskmgr {

enum class Priority { Low = 0, Medium = 1, High = 2 };

std::string to_string(Priority priority);

// Accepts "low"/"l", "medium"/"med"/"m", "high"/"h" (case-insensitive).
std::optional<Priority> parse_priority(const std::string& text);

// Checks a calendar date in YYYY-MM-DD format, including leap years.
bool is_valid_date(const std::string& date);

struct Task {
    int id{};
    std::string title;
    Priority priority{Priority::Medium};
    bool done{false};
    std::optional<std::string> due;  // YYYY-MM-DD
};

}  // namespace taskmgr
