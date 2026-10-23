#include <gtest/gtest.h>

#include <memory>

#include "taskmgr/Errors.h"
#include "taskmgr/TaskManager.h"

using namespace taskmgr;

namespace {

struct Fixture : ::testing::Test {
    Fixture() {
        auto storage = std::make_unique<MemoryStorage>();
        raw = storage.get();
        manager = std::make_unique<TaskManager>(std::move(storage));
    }
    MemoryStorage* raw{};  // owned by manager
    std::unique_ptr<TaskManager> manager;
};

}  // namespace

TEST(TaskManagerCtor, RejectsNullStorage) {
    EXPECT_THROW(TaskManager(nullptr), std::invalid_argument);
}

TEST_F(Fixture, AddAssignsIncreasingIds) {
    EXPECT_EQ(manager->add("first").id, 1);
    EXPECT_EQ(manager->add("second").id, 2);
    EXPECT_EQ(manager->size(), 2u);
}

TEST_F(Fixture, AddTrimsTitleAndStoresFields) {
    const Task t = manager->add("  buy milk \t", Priority::High, std::string("2026-12-24"));
    EXPECT_EQ(t.title, "buy milk");
    EXPECT_EQ(t.priority, Priority::High);
    EXPECT_FALSE(t.done);
    EXPECT_EQ(t.due, std::string("2026-12-24"));
}

TEST_F(Fixture, AddRejectsEmptyTitleAndBadDate) {
    EXPECT_THROW(manager->add(""), TaskError);
    EXPECT_THROW(manager->add("   "), TaskError);
    EXPECT_THROW(manager->add("x", Priority::Low, std::string("2026-02-30")), TaskError);
    EXPECT_EQ(manager->size(), 0u);
}

TEST_F(Fixture, IdsAreNotReusedAfterRemoval) {
    manager->add("a");
    manager->add("b");
    manager->remove(2);
    EXPECT_EQ(manager->add("c").id, 3);
}

TEST_F(Fixture, RemoveUnknownIdThrows) {
    EXPECT_THROW(manager->remove(42), TaskError);
}

TEST_F(Fixture, MarkDoneAndGet) {
    const int id = manager->add("task").id;
    manager->mark_done(id);
    EXPECT_TRUE(manager->get(id).done);
    EXPECT_THROW(manager->mark_done(99), TaskError);
    EXPECT_THROW(manager->get(99), TaskError);
}

TEST_F(Fixture, EditTitle) {
    const int id = manager->add("old").id;
    manager->edit_title(id, "  new ");
    EXPECT_EQ(manager->get(id).title, "new");
    EXPECT_THROW(manager->edit_title(id, " "), TaskError);
    EXPECT_THROW(manager->edit_title(99, "x"), TaskError);
}

TEST_F(Fixture, ListOrdersByDoneThenPriorityThenId) {
    manager->add("low", Priority::Low);        // 1
    manager->add("high-a", Priority::High);    // 2
    manager->add("medium", Priority::Medium);  // 3
    manager->add("high-b", Priority::High);    // 4
    manager->mark_done(2);

    const auto tasks = manager->list();
    ASSERT_EQ(tasks.size(), 4u);
    EXPECT_EQ(tasks[0].id, 4);  // not done, high
    EXPECT_EQ(tasks[1].id, 3);  // not done, medium
    EXPECT_EQ(tasks[2].id, 1);  // not done, low
    EXPECT_EQ(tasks[3].id, 2);  // done goes last
}

TEST_F(Fixture, ListFilters) {
    manager->add("a", Priority::Low);
    manager->add("b", Priority::High);
    manager->add("c", Priority::High);
    manager->mark_done(3);

    TaskFilter todo;
    todo.done = false;
    EXPECT_EQ(manager->list(todo).size(), 2u);

    TaskFilter done;
    done.done = true;
    ASSERT_EQ(manager->list(done).size(), 1u);
    EXPECT_EQ(manager->list(done)[0].id, 3);

    TaskFilter high;
    high.priority = Priority::High;
    EXPECT_EQ(manager->list(high).size(), 2u);

    TaskFilter both;
    both.priority = Priority::High;
    both.done = false;
    ASSERT_EQ(manager->list(both).size(), 1u);
    EXPECT_EQ(manager->list(both)[0].id, 2);
}

TEST_F(Fixture, SearchIsCaseInsensitiveSubstring) {
    manager->add("Buy MILK");
    manager->add("Call mom");
    manager->add("buy bread");

    EXPECT_EQ(manager->search("buy").size(), 2u);
    EXPECT_EQ(manager->search("MiLk").size(), 1u);
    EXPECT_TRUE(manager->search("zzz").empty());
}

TEST_F(Fixture, SaveWritesToStorageAndLoadRestoresNextId) {
    manager->add("a");
    manager->add("b");
    manager->save();
    EXPECT_EQ(raw->data().size(), 2u);

    // A second manager over the same data continues numbering after the max id.
    auto storage = std::make_unique<MemoryStorage>();
    storage->save(raw->data());
    TaskManager restored(std::move(storage));
    restored.load();
    EXPECT_EQ(restored.size(), 2u);
    EXPECT_EQ(restored.add("c").id, 3);
}
