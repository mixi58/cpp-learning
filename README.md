# cpp-task-manager

[![CI](https://github.com/mixi58/cpp-learning/actions/workflows/ci.yml/badge.svg)](https://github.com/mixi58/cpp-learning/actions/workflows/ci.yml)

An interactive command-line task manager written in modern C++17.
Tasks have a title, priority and optional due date, and are saved to a plain text file between runs.

The project is small on purpose: the goal is clean structure, tests and a reproducible build rather than many features.

## Example session

```
Task manager (0 tasks loaded). Type 'help'.
> add Write unit tests -p high -d 2026-11-01
Added task #1
> add "Buy milk"
Added task #2
> add Learn CMake -p low
Added task #3
> list
ID   PRIORITY DUE         DONE  TITLE
1    high     2026-11-01  [ ]   Write unit tests
2    medium   -           [ ]   Buy milk
3    low      -           [ ]   Learn CMake
> done 1
Task #1 marked as done
> search milk
ID   PRIORITY DUE         DONE  TITLE
2    medium   -           [ ]   Buy milk
> add x -d 2026-02-30
Error: invalid date (expected YYYY-MM-DD): 2026-02-30
> quit
```

## Commands

| Command | Description |
|---|---|
| `add <title> [-p low\|medium\|high] [-d YYYY-MM-DD]` | add a task |
| `list [--todo \| --done] [-p <priority>]` | show tasks (not done first, then by priority) |
| `done <id>` | mark a task as done |
| `edit <id> <new title>` | change the title |
| `rm <id>` | delete a task |
| `search <text>` | case-insensitive search in titles |
| `help`, `quit` | help / exit |

Use double quotes to keep several words together when needed: `add "Call mom" -p high`.

## Build and run

Requirements: a C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+) and CMake 3.16+.
Unit tests use GoogleTest, which CMake downloads automatically (internet needed for the first configure).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure   # CMake 3.20+

# run (Linux/macOS)
./build/taskmgr
# run (Windows)
build\Release\taskmgr.exe
```

Pass a file name to use a different task file: `taskmgr work.tsv` (default is `tasks.tsv` in the current directory).
To build without tests: `cmake -S . -B build -DTASKMGR_BUILD_TESTS=OFF`.

## Project structure

```
include/taskmgr/   public headers
  Task.h           Task, Priority, date validation
  Storage.h        IStorage interface, FileStorage, MemoryStorage
  TaskManager.h    business logic (add, remove, filter, search, sort)
  Cli.h            command parser and interpreter
  Errors.h         TaskError, StorageError
src/               implementations and main()
tests/             GoogleTest unit tests
.github/workflows/ CI: build and test on Linux, Windows and macOS
```

## Design notes

- **Dependency injection.** `TaskManager` owns an `std::unique_ptr<IStorage>`. Production code passes `FileStorage`, tests pass `MemoryStorage`, so business logic is tested without touching the disk.
- **`Cli` writes to any `std::ostream`**, so command handling is tested by comparing the produced text, with no real console involved.
- **Safe saving.** `FileStorage` writes a temporary file and then renames it over the real one, so an interrupted save does not destroy existing data.
- **Validation and errors.** Invalid input throws `TaskError`; broken data files throw `StorageError` with file name and line number. `Cli::execute` catches errors and prints them, so a typo never crashes the program.
- **Ids are never reused** after a task is deleted.
- **Modern C++ used:** `enum class`, `std::optional`, `std::unique_ptr`, `std::filesystem`, `std::from_chars`, structured bindings, STL algorithms (`copy_if`, `stable_sort`, `find_if`), RAII, `const` correctness. No raw `new`/`delete`.

## Storage format

A text file with one task per line, fields separated by tabs:

```
# taskmgr v1
1	1	high	2026-11-01	Write unit tests
2	0	medium	-	Buy oat milk
```

Fields: id, done (0/1), priority, due date or `-`, title (tabs, newlines and backslashes are escaped).

## Possible improvements

- Tags and sub-tasks
- JSON export
- Sort options and due-date reminders
- Parsing arguments from the command line (`taskmgr add ...`) in addition to interactive mode

## License

MIT, see [LICENSE](LICENSE).
