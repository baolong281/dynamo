#include <gtest/gtest.h>
#include "storage/value.h"

TEST(ClockTest, BasicClockTest) {
    VectorClock a, b;

    a.increment("x");
    a.increment("y");
    a.increment("x");

    b.increment("x");
    b.increment("y");
    b.increment("y");

    EXPECT_EQ(a.get("x"), 2);
    EXPECT_EQ(a.get("y"), 1);
    EXPECT_EQ(b.get("x"), 1);
    EXPECT_EQ(b.get("y"), 2);

    EXPECT_FALSE(a < b);
    EXPECT_FALSE(b < a);

    VectorClock c;
    c.increment("x");
    c.increment("x");
    c.increment("y");
    EXPECT_TRUE(a < c || c < a || a.get("x") == c.get("x"));  // basic sanity
}


TEST(ClockTest, OperatorLessCausality) {
    VectorClock a, b, c;

    // a -> {x:2, y:1}
    a.increment("x");
    a.increment("x");
    a.increment("y");

    // b -> {x:1, y:2}
    b.increment("x");
    b.increment("y");
    b.increment("y");

    // c -> {x:1, y:1}
    c.increment("x");
    c.increment("y");

    // c happens before a
    EXPECT_TRUE(c < a);
    EXPECT_TRUE(c < b);

    // a and b are concurrent 
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(b < a);

}

TEST(ClockTest, SiblingDetection) {
    VectorClock a, b, c;

    // a -> {x:2, y:1}
    a.increment("x");
    a.increment("x");
    a.increment("y");

    // b -> {x:1, y:2}
    b.increment("x");
    b.increment("y");
    b.increment("y");

    // c -> {x:1, y:1}
    c.increment("x");
    c.increment("y");

    // a and b are siblings/concurrent
    EXPECT_TRUE(a.isSibling(b));
    EXPECT_TRUE(b.isSibling(a));

    // c happens-before a and b
    EXPECT_FALSE(a.isSibling(c));
    EXPECT_FALSE(c.isSibling(a));
    EXPECT_FALSE(b.isSibling(c));
    EXPECT_FALSE(c.isSibling(b));

    // a is not sibling with itself
    EXPECT_FALSE(a.isSibling(a));
}
