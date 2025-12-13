#include <gtest/gtest.h>
#include "storage/serializer.h"
#include "storage/value.h"

TEST(SerializerTest, BinaryRoundTripMultipleValues) {
    VectorClock clock1, clock2;
    clock1.increment("one");
    clock1.increment("one");  
    clock2.increment("two"); 

    Value val1{"first", clock1};
    Value val2{"second", clock2};
    Value val3{"third", VectorClock{}};

    ValueList original{val1, val2, val3};

    auto binary = Serializer::toBinary(original);
    auto deserialized = Serializer::fromBinary<ValueList>(binary);

    ASSERT_EQ(original.size(), deserialized.size());

    EXPECT_EQ(deserialized[0].data_, "first");
    EXPECT_EQ(deserialized[0].clock_.get("one"), 2);

    EXPECT_EQ(deserialized[1].data_, "second");
    EXPECT_EQ(deserialized[1].clock_.get("two"), 1);

    EXPECT_EQ(deserialized[2].data_, "third");
    EXPECT_EQ(deserialized[2].clock_.get("one"), 0);
    EXPECT_EQ(deserialized[2].clock_.get("two"), 0);
}

TEST(SerializerTest, EmptyValueList) {
    ValueList empty{};
    auto binary = Serializer::toBinary(empty);
    auto deserialized = Serializer::fromBinary<ValueList>(binary);
    EXPECT_TRUE(deserialized.empty());
}
