#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "taskmgr/TaskManager.h"

namespace taskmgr {

// Splits a line into words. Double quotes group words: add "buy milk" -> 2 tokens.
// Throws TaskError if a quote is not closed.
std::vector<std::string> tokenize(const std::string& line);

// Command interpreter. Works with any std::ostream, which makes it easy to test.
class Cli {
public:
    Cli(TaskManager& manager, std::ostream& out);

    // Executes one command line. Returns false when the user wants to quit.
    // Errors are reported to the output stream and never escape this method.
    bool execute(const std::string& line);

private:
    void cmd_add(const std::vector<std::string>& args);
    void cmd_list(const std::vector<std::string>& args);
    void cmd_done(const std::vector<std::string>& args);
    void cmd_remove(const std::vector<std::string>& args);
    void cmd_edit(const std::vector<std::string>& args);
    void cmd_search(const std::vector<std::string>& args);
    void print_help();
    void print_tasks(const std::vector<Task>& tasks);

    TaskManager& manager_;
    std::ostream& out_;
};

}  // namespace taskmgr
