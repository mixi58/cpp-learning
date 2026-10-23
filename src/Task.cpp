#include "taskmgr/Task.h"

#include <algorithm>
#include <array>
#include <cctype>

namespace taskmgr {

std::string to_string(Priority priority) {
    switch (priority) {
        case Priority::Low:
            return "low";
        case Priority::Medium:
            return "medium";
        case Priority::High:
            return "high";
    }
    return "medium";
}

std::optional<Priority> parse_priority(const std::string& text) {
    std::string s = text;
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (s == "low" || s == "l") return Priority::Low;
    if (s == "medium" || s == "med" || s == "m") return Priority::Medium;
    if (s == "high" || s == "h") return Priority::High;
    return std::nullopt;
}

bool is_valid_date(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;

    for (std::size_t i : {0u, 1u, 2u, 3u, 5u, 6u, 8u, 9u}) {
        if (!std::isdigit(static_cast<unsigned char>(date[i]))) return false;
    }

    const int year = std::stoi(date.substr(0, 4));
    const int month = std::stoi(date.substr(5, 2));
    const int day = std::stoi(date.substr(8, 2));

    if (month < 1 || month > 12 || day < 1) return false;

    static constexpr std::array<int, 12> days_in_month{31, 28, 31, 30, 31, 30,
                                                       31, 31, 30, 31, 30, 31};
    const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    const int max_day = days_in_month[static_cast<std::size_t>(month - 1)] +
                        ((month == 2 && leap) ? 1 : 0);
    return day <= max_day;
}

}  // namespace taskmgr
