#include <gtest/gtest.h>

#include <memory>
#include <sstream>

#include "taskmgr/Cli.h"
#include "taskmgr/Errors.h"

using namespace taskmgr;

TEST(Tokenize, SplitsOnWhitespace) {
    const auto t = tokenize("  add   buy   milk ");
    ASSERT_EQ(t.size(), 3u);
    EXPECT_EQ(t[0], "add");
    EXPECT_EQ(t[2], "milk");
}

TEST(Tokenize, QuotesGroupWords) {
    const auto t = tokenize("add \"buy fresh milk\" -p high");
    ASSERT_EQ(t.size(), 4u);
    EXPECT_EQ(t[1], "buy fresh milk");
}

TEST(Tokenize, EmptyQuotesGiveEmptyToken) {
    const auto t = tokenize("a \"\" b");
    ASSERT_EQ(t.size(), 3u);
    EXPECT_EQ(t[1], "");
}

TEST(Tokenize, EmptyLineGivesNoTokens) { EXPECT_TRUE(tokenize("   ").empty()); }

TEST(Tokenize, UnclosedQuoteThrows) { EXPECT_THROW(tokenize("add \"oops"), TaskError); }

namespace {

struct CliFixture : ::testing::Test {
    CliFixture()
        : manager(std::make_unique<MemoryStorage>()), cli(manager, out) {}

    std::string run(const std::string& line) {
        out.str("");
        cli.execute(line);
        return out.str();
    }

    TaskManager manager;
    std::ostringstream out;
    Cli cli;
};

}  // namespace

TEST_F(CliFixture, QuitAndExitStopTheLoop) {
    EXPECT_FALSE(cli.execute("quit"));
    EXPECT_FALSE(cli.execute("EXIT"));
    EXPECT_TRUE(cli.execute("help"));
    EXPECT_TRUE(cli.execute(""));
}

TEST_F(CliFixture, AddWithOptions) {
    EXPECT_NE(run("add Write report -p high -d 2026-11-30").find("Added task #1"),
              std::string::npos);
    const Task t = manager.get(1);
    EXPECT_EQ(t.title, "Write report");
    EXPECT_EQ(t.priority, Priority::High);
    EXPECT_EQ(t.due, std::string("2026-11-30"));
}

TEST_F(CliFixture, AddReportsErrorsInsteadOfThrowing) {
    EXPECT_NE(run("add").find("Error:"), std::string::npos);
    EXPECT_NE(run("add x -p urgent").find("Error:"), std::string::npos);
    EXPECT_NE(run("add x -d 2026-02-30").find("Error:"), std::string::npos);
    EXPECT_NE(run("add x -p").find("missing value"), std::string::npos);
    EXPECT_EQ(manager.size(), 0u);
}

TEST_F(CliFixture, ListShowsTasksAndEmptyMessage) {
    EXPECT_NE(run("list").find("No tasks."), std::string::npos);

    run("add Alpha");
    run("add Beta -p high");
    const std::string output = run("list");
    EXPECT_NE(output.find("Alpha"), std::string::npos);
    EXPECT_LT(output.find("Beta"), output.find("Alpha"));  // high priority first
}

TEST_F(CliFixture, DoneRemoveEditSearch) {
    run("add Buy milk");
    run("add Call mom");

    EXPECT_NE(run("done 1").find("marked as done"), std::string::npos);
    EXPECT_TRUE(manager.get(1).done);

    EXPECT_NE(run("edit 2 Call dad").find("updated"), std::string::npos);
    EXPECT_EQ(manager.get(2).title, "Call dad");

    EXPECT_NE(run("search DAD").find("Call dad"), std::string::npos);

    EXPECT_NE(run("rm 1").find("removed"), std::string::npos);
    EXPECT_EQ(manager.size(), 1u);
}

TEST_F(CliFixture, BadIdsAndUnknownCommandsAreReported) {
    EXPECT_NE(run("done abc").find("invalid id"), std::string::npos);
    EXPECT_NE(run("done 99").find("not found"), std::string::npos);
    EXPECT_NE(run("rm").find("usage"), std::string::npos);
    EXPECT_NE(run("dance").find("Unknown command"), std::string::npos);
    EXPECT_NE(run("list --bogus").find("unknown option"), std::string::npos);
}

TEST_F(CliFixture, ListFiltersByStatus) {
    run("add One");
    run("add Two");
    run("done 1");
    const std::string done = run("list --done");
    EXPECT_NE(done.find("One"), std::string::npos);
    EXPECT_EQ(done.find("Two"), std::string::npos);
    const std::string todo = run("list --todo");
    EXPECT_NE(todo.find("Two"), std::string::npos);
    EXPECT_EQ(todo.find("One"), std::string::npos);
}
