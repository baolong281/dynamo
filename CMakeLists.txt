cmake_minimum_required(VERSION 3.10)
project(Dynamo VERSION 1.0 LANGUAGES CXX)

option(BUILD_TESTING "Build tests" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED)

find_path(HTTPLIB_INCLUDE_DIR
    NAMES httplib.h
    HINTS /opt/homebrew/Cellar/cpp-httplib/0.26.0/
)

set(OPENSSL_ROOT_DIR /opt/homebrew/Cellar/openssl@3/3.5.2)
set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT_DIR}/include)
set(OPENSSL_LIB_DIR ${OPENSSL_ROOT_DIR}/lib)

find_library(OPENSSL_CRYPTO_LIBRARY crypto HINTS ${OPENSSL_LIB_DIR})
find_library(OPENSSL_SSL_LIBRARY ssl HINTS ${OPENSSL_LIB_DIR})

add_library(dynamo STATIC
    src/hash_ring/hash_ring.cpp
)

add_library(Dynamo::dynamo ALIAS dynamo)

target_include_directories(dynamo
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${Boost_INCLUDE_DIRS}
        ${HTTPLIB_INCLUDE_DIR}
        ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(dynamo
    PUBLIC
        ${OPENSSL_CRYPTO_LIBRARY}
        ${OPENSSL_SSL_LIBRARY}
)

add_executable(app
    src/main.cpp
)

target_link_libraries(app
    PRIVATE Dynamo::dynamo
)

install(TARGETS app
    RUNTIME DESTINATION bin
)

install(TARGETS dynamo
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
