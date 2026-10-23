#include "taskmgr/Storage.h"

#include <charconv>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#include "taskmgr/Errors.h"

namespace taskmgr {
namespace {

constexpr char kHeader[] = "# taskmgr v1";

std::string escape(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (char c : text) {
        switch (c) {
            case '\\':
                result += "\\\\";
                break;
            case '\t':
                result += "\\t";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            default:
                result += c;
        }
    }
    return result;
}

std::string unescape(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '\\') {
            result += text[i];
            continue;
        }
        if (++i >= text.size()) throw StorageError("dangling backslash in title");
        switch (text[i]) {
            case '\\':
                result += '\\';
                break;
            case 't':
                result += '\t';
                break;
            case 'n':
                result += '\n';
                break;
            case 'r':
                result += '\r';
                break;
            default:
                throw StorageError("unknown escape sequence in title");
        }
    }
    return result;
}

std::vector<std::string> split_tabs(const std::string& line) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true) {
        const std::size_t pos = line.find('\t', start);
        if (pos == std::string::npos) {
            parts.push_back(line.substr(start));
            return parts;
        }
        parts.push_back(line.substr(start, pos - start));
        start = pos + 1;
    }
}

int parse_int(const std::string& text) {
    int value = 0;
    const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || ptr != text.data() + text.size()) {
        throw StorageError("invalid number: " + text);
    }
    return value;
}

Task parse_line(const std::string& line) {
    const auto fields = split_tabs(line);
    if (fields.size() != 5) throw StorageError("expected 5 fields, got " + std::to_string(fields.size()));

    Task task;
    task.id = parse_int(fields[0]);
    if (task.id <= 0) throw StorageError("id must be positive");

    if (fields[1] != "0" && fields[1] != "1") throw StorageError("done flag must be 0 or 1");
    task.done = fields[1] == "1";

    const auto priority = parse_priority(fields[2]);
    if (!priority) throw StorageError("unknown priority: " + fields[2]);
    task.priority = *priority;

    if (fields[3] != "-") {
        if (!is_valid_date(fields[3])) throw StorageError("invalid date: " + fields[3]);
        task.due = fields[3];
    }

    task.title = unescape(fields[4]);
    return task;
}

}  // namespace

FileStorage::FileStorage(std::filesystem::path path) : path_(std::move(path)) {}

std::vector<Task> FileStorage::load() {
    std::error_code ec;
    if (!std::filesystem::exists(path_, ec)) return {};

    std::ifstream in(path_);
    if (!in) throw StorageError("cannot open file for reading: " + path_.string());

    std::vector<Task> tasks;
    std::string line;
    int line_number = 0;
    while (std::getline(in, line)) {
        ++line_number;
        if (!line.empty() && line.back() == '\r') line.pop_back();  // Windows line endings
        if (line.empty() || line[0] == '#') continue;
        try {
            tasks.push_back(parse_line(line));
        } catch (const StorageError& e) {
            throw StorageError(path_.string() + ":" + std::to_string(line_number) + ": " + e.what());
        }
    }
    return tasks;
}

void FileStorage::save(const std::vector<Task>& tasks) {
    if (path_.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path_.parent_path(), ec);
    }

    std::filesystem::path tmp = path_;
    tmp += ".tmp";

    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) throw StorageError("cannot open file for writing: " + tmp.string());

        out << kHeader << '\n';
        for (const Task& t : tasks) {
            out << t.id << '\t' << (t.done ? 1 : 0) << '\t' << to_string(t.priority) << '\t'
                << (t.due ? *t.due : std::string("-")) << '\t' << escape(t.title) << '\n';
        }
        out.flush();
        if (!out) throw StorageError("failed to write file: " + tmp.string());
    }

    std::error_code ec;
    std::filesystem::rename(tmp, path_, ec);
    if (ec) throw StorageError("cannot replace " + path_.string() + ": " + ec.message());
}

}  // namespace taskmgr
