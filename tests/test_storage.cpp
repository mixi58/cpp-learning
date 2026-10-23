#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "taskmgr/Errors.h"
#include "taskmgr/Storage.h"

using namespace taskmgr;
namespace fs = std::filesystem;

namespace {

// Creates a unique temporary directory and removes it after the test.
class FileStorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        dir_ = fs::temp_directory_path() / (std::string("taskmgr_") + info->name());
        fs::remove_all(dir_);
        fs::create_directories(dir_);
        file_ = dir_ / "tasks.tsv";
    }
    void TearDown() override { fs::remove_all(dir_); }

    void write_raw(const std::string& content) {
        std::ofstream out(file_, std::ios::binary);
        out << content;
    }

    fs::path dir_;
    fs::path file_;
};

}  // namespace

TEST_F(FileStorageTest, MissingFileMeansNoTasks) {
    FileStorage storage(file_);
    EXPECT_TRUE(storage.load().empty());
}

TEST_F(FileStorageTest, RoundTripKeepsAllFields) {
    std::vector<Task> tasks;
    tasks.push_back(Task{1, "Write tests", Priority::High, false, std::string("2026-11-01")});
    tasks.push_back(Task{7, "Ship it", Priority::Low, true, std::nullopt});

    FileStorage storage(file_);
    storage.save(tasks);
    const auto loaded = storage.load();

    ASSERT_EQ(loaded.size(), 2u);
    EXPECT_EQ(loaded[0].id, 1);
    EXPECT_EQ(loaded[0].title, "Write tests");
    EXPECT_EQ(loaded[0].priority, Priority::High);
    EXPECT_FALSE(loaded[0].done);
    EXPECT_EQ(loaded[0].due, std::string("2026-11-01"));
    EXPECT_EQ(loaded[1].id, 7);
    EXPECT_TRUE(loaded[1].done);
    EXPECT_FALSE(loaded[1].due.has_value());
}

TEST_F(FileStorageTest, SpecialCharactersInTitleSurviveRoundTrip) {
    const std::string title = "tab\there, new\nline, back\\slash, cr\r!";
    FileStorage storage(file_);
    storage.save({Task{1, title, Priority::Medium, false, std::nullopt}});

    const auto loaded = storage.load();
    ASSERT_EQ(loaded.size(), 1u);
    EXPECT_EQ(loaded[0].title, title);
}

TEST_F(FileStorageTest, SaveReplacesPreviousContentAndLeavesNoTempFile) {
    FileStorage storage(file_);
    storage.save({Task{1, "old", Priority::Low, false, std::nullopt}});
    storage.save({Task{2, "new", Priority::Low, false, std::nullopt}});

    const auto loaded = storage.load();
    ASSERT_EQ(loaded.size(), 1u);
    EXPECT_EQ(loaded[0].title, "new");
    EXPECT_FALSE(fs::exists(file_.string() + ".tmp"));
}

TEST_F(FileStorageTest, ReadsWindowsLineEndings) {
    write_raw("# taskmgr v1\r\n1\t0\thigh\t-\tHello\r\n");
    FileStorage storage(file_);
    const auto loaded = storage.load();
    ASSERT_EQ(loaded.size(), 1u);
    EXPECT_EQ(loaded[0].title, "Hello");
}

TEST_F(FileStorageTest, MalformedLinesThrowWithLineNumber) {
    FileStorage storage(file_);

    write_raw("1\t0\thigh\n");  // too few fields
    EXPECT_THROW(storage.load(), StorageError);

    write_raw("x\t0\thigh\t-\tTitle\n");  // id is not a number
    EXPECT_THROW(storage.load(), StorageError);

    write_raw("1\t2\thigh\t-\tTitle\n");  // done flag out of range
    EXPECT_THROW(storage.load(), StorageError);

    write_raw("1\t0\turgent\t-\tTitle\n");  // unknown priority
    EXPECT_THROW(storage.load(), StorageError);

    write_raw("1\t0\thigh\t2026-02-30\tTitle\n");  // impossible date
    EXPECT_THROW(storage.load(), StorageError);

    write_raw("# header\n1\t0\thigh\t-\tOk\n2\tbroken\n");
    try {
        storage.load();
        FAIL() << "expected StorageError";
    } catch (const StorageError& e) {
        EXPECT_NE(std::string(e.what()).find(":3:"), std::string::npos) << e.what();
    }
}

TEST(MemoryStorage, StoresWhatItIsGiven) {
    MemoryStorage storage;
    storage.save({Task{1, "a", Priority::Low, false, std::nullopt}});
    EXPECT_EQ(storage.load().size(), 1u);
    EXPECT_EQ(storage.data().front().title, "a");
}
