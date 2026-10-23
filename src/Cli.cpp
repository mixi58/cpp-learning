#include "taskmgr/Cli.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <iomanip>
#include <string>

#include "taskmgr/Errors.h"

namespace taskmgr {
namespace {

std::string to_lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

int parse_id(const std::string& text) {
    int id = 0;
    const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), id);
    if (ec != std::errc{} || ptr != text.data() + text.size()) {
        throw TaskError("invalid id: " + text);
    }
    return id;
}

const std::string& option_value(const std::vector<std::string>& args, std::size_t& i) {
    if (i + 1 >= args.size()) throw TaskError("missing value for " + args[i]);
    return args[++i];
}

Priority require_priority(const std::string& text) {
    const auto priority = parse_priority(text);
    if (!priority) throw TaskError("unknown priority (use low, medium or high): " + text);
    return *priority;
}

std::string join(const std::vector<std::string>& words, std::size_t from) {
    std::string result;
    for (std::size_t i = from; i < words.size(); ++i) {
        if (!result.empty()) result += ' ';
        result += words[i];
    }
    return result;
}

}  // namespace

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;
    bool has_token = false;

    for (char c : line) {
        if (c == '"') {
            in_quotes = !in_quotes;
            has_token = true;  // "" produces an empty token
        } else if (std::isspace(static_cast<unsigned char>(c)) && !in_quotes) {
            if (has_token) {
                tokens.push_back(current);
                current.clear();
                has_token = false;
            }
        } else {
            current += c;
            has_token = true;
        }
    }

    if (in_quotes) throw TaskError("unclosed quote");
    if (has_token) tokens.push_back(current);
    return tokens;
}

Cli::Cli(TaskManager& manager, std::ostream& out) : manager_(manager), out_(out) {}

bool Cli::execute(const std::string& line) {
    try {
        const auto tokens = tokenize(line);
        if (tokens.empty()) return true;

        const std::string command = to_lower(tokens[0]);
        const std::vector<std::string> args(tokens.begin() + 1, tokens.end());

        if (command == "quit" || command == "exit") return false;

        if (command == "add") {
            cmd_add(args);
        } else if (command == "list" || command == "ls") {
            cmd_list(args);
        } else if (command == "done") {
            cmd_done(args);
        } else if (command == "rm" || command == "remove") {
            cmd_remove(args);
        } else if (command == "edit") {
            cmd_edit(args);
        } else if (command == "search") {
            cmd_search(args);
        } else if (command == "help") {
            print_help();
        } else {
            out_ << "Unknown command: " << command << " (type 'help')\n";
        }
    } catch (const std::exception& e) {
        out_ << "Error: " << e.what() << '\n';
    }
    return true;
}

void Cli::cmd_add(const std::vector<std::string>& args) {
    std::vector<std::string> title_words;
    Priority priority = Priority::Medium;
    std::optional<std::string> due;

    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "-p" || args[i] == "--priority") {
            priority = require_priority(option_value(args, i));
        } else if (args[i] == "-d" || args[i] == "--due") {
            due = option_value(args, i);
        } else {
            title_words.push_back(args[i]);
        }
    }

    const Task task = manager_.add(join(title_words, 0), priority, due);
    manager_.save();
    out_ << "Added task #" << task.id << '\n';
}

void Cli::cmd_list(const std::vector<std::string>& args) {
    TaskFilter filter;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--done") {
            filter.done = true;
        } else if (args[i] == "--todo") {
            filter.done = false;
        } else if (args[i] == "-p" || args[i] == "--priority") {
            filter.priority = require_priority(option_value(args, i));
        } else {
            throw TaskError("unknown option: " + args[i]);
        }
    }
    print_tasks(manager_.list(filter));
}

void Cli::cmd_done(const std::vector<std::string>& args) {
    if (args.size() != 1) throw TaskError("usage: done <id>");
    manager_.mark_done(parse_id(args[0]));
    manager_.save();
    out_ << "Task #" << args[0] << " marked as done\n";
}

void Cli::cmd_remove(const std::vector<std::string>& args) {
    if (args.size() != 1) throw TaskError("usage: rm <id>");
    manager_.remove(parse_id(args[0]));
    manager_.save();
    out_ << "Task #" << args[0] << " removed\n";
}

void Cli::cmd_edit(const std::vector<std::string>& args) {
    if (args.size() < 2) throw TaskError("usage: edit <id> <new title>");
    manager_.edit_title(parse_id(args[0]), join(args, 1));
    manager_.save();
    out_ << "Task #" << args[0] << " updated\n";
}

void Cli::cmd_search(const std::vector<std::string>& args) {
    if (args.empty()) throw TaskError("usage: search <text>");
    print_tasks(manager_.search(join(args, 0)));
}

void Cli::print_help() {
    out_ << "Commands:\n"
            "  add <title> [-p low|medium|high] [-d YYYY-MM-DD]   add a task\n"
            "  list [--todo | --done] [-p <priority>]             show tasks\n"
            "  done <id>                                          mark as done\n"
            "  edit <id> <new title>                              change title\n"
            "  rm <id>                                            delete a task\n"
            "  search <text>                                      find by title\n"
            "  help                                               this help\n"
            "  quit                                               exit\n";
}

void Cli::print_tasks(const std::vector<Task>& tasks) {
    if (tasks.empty()) {
        out_ << "No tasks.\n";
        return;
    }
    out_ << std::left << std::setw(5) << "ID" << std::setw(9) << "PRIORITY" << std::setw(12)
         << "DUE" << std::setw(6) << "DONE" << "TITLE\n";
    for (const Task& t : tasks) {
        out_ << std::left << std::setw(5) << t.id << std::setw(9) << to_string(t.priority)
             << std::setw(12) << (t.due ? *t.due : std::string("-")) << std::setw(6)
             << (t.done ? "[x]" : "[ ]") << t.title << '\n';
    }
}

}  // namespace taskmgr
