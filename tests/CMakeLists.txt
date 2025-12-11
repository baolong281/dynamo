find_package(GTest REQUIRED)

include(GoogleTest)

add_executable(test_hash_ring
    hashing/hash_ring_test.cc
)

target_link_libraries(test_hash_ring
    PRIVATE
        Dynamo::dynamo
        GTest::gtest
        GTest::gtest_main
)

target_link_libraries(test_hash_ring
    PRIVATE
        Dynamo::dynamo
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(test_hash_ring)

add_executable(test_serializer
    serialization/serializer_test.cc
)

target_link_libraries(test_serializer
    PRIVATE
        Dynamo::dynamo
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(test_serializer)
