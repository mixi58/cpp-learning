#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "taskmgr/Cli.h"
#include "taskmgr/Storage.h"
#include "taskmgr/TaskManager.h"

int main(int argc, char* argv[]) {
    const std::string first_arg = argc > 1 ? argv[1] : "";
    if (first_arg == "--help" || first_arg == "-h") {
        std::cout << "Usage: taskmgr [tasks-file]\n"
                     "Interactive task manager. Default file: tasks.tsv\n";
        return 0;
    }

    const std::filesystem::path file = argc > 1 ? argv[1] : "tasks.tsv";

    try {
        taskmgr::TaskManager manager(std::make_unique<taskmgr::FileStorage>(file));
        manager.load();

        taskmgr::Cli cli(manager, std::cout);
        std::cout << "Task manager (" << manager.size() << " tasks loaded). Type 'help'.\n";

        std::string line;
        while (std::cout << "> ", std::getline(std::cin, line)) {
            if (!cli.execute(line)) break;
        }
        std::cout << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
