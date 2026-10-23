#include <gtest/gtest.h>

#include "taskmgr/Task.h"

using namespace taskmgr;

TEST(Priority, ParsesFullAndShortNamesCaseInsensitively) {
    EXPECT_EQ(parse_priority("low"), Priority::Low);
    EXPECT_EQ(parse_priority("L"), Priority::Low);
    EXPECT_EQ(parse_priority("Medium"), Priority::Medium);
    EXPECT_EQ(parse_priority("med"), Priority::Medium);
    EXPECT_EQ(parse_priority("HIGH"), Priority::High);
    EXPECT_EQ(parse_priority("h"), Priority::High);
}

TEST(Priority, RejectsUnknownText) {
    EXPECT_FALSE(parse_priority("urgent").has_value());
    EXPECT_FALSE(parse_priority("").has_value());
}

TEST(Priority, ToStringRoundTrips) {
    for (Priority p : {Priority::Low, Priority::Medium, Priority::High}) {
        EXPECT_EQ(parse_priority(to_string(p)), p);
    }
}

TEST(Date, AcceptsValidDates) {
    EXPECT_TRUE(is_valid_date("2026-10-22"));
    EXPECT_TRUE(is_valid_date("2024-02-29"));  // leap year
    EXPECT_TRUE(is_valid_date("2000-02-29"));  // divisible by 400
    EXPECT_TRUE(is_valid_date("2026-12-31"));
}

TEST(Date, RejectsInvalidDates) {
    EXPECT_FALSE(is_valid_date("2025-02-29"));  // not a leap year
    EXPECT_FALSE(is_valid_date("1900-02-29"));  // divisible by 100, not by 400
    EXPECT_FALSE(is_valid_date("2026-13-01"));
    EXPECT_FALSE(is_valid_date("2026-00-10"));
    EXPECT_FALSE(is_valid_date("2026-04-31"));
    EXPECT_FALSE(is_valid_date("2026-04-00"));
}

TEST(Date, RejectsBadFormat) {
    EXPECT_FALSE(is_valid_date(""));
    EXPECT_FALSE(is_valid_date("2026/10/22"));
    EXPECT_FALSE(is_valid_date("22-10-2026"));
    EXPECT_FALSE(is_valid_date("2026-1-2"));
    EXPECT_FALSE(is_valid_date("abcd-ef-gh"));
}
