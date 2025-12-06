cmake_minimum_required(VERSION 3.10)
project(MyFixApp VERSION 1.0 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Boost REQUIRED)

add_executable(app
    src/main.cpp
)

target_include_directories(app
    PRIVATE include ${Boost_INCLUDE_DIRS}
)

# target_link_libraries(app PRIVATE
#     ${POCO_FOUNDATION_LIBRARY}
#     ${POCO_NET_LIBRARY}
#     ${POCO_NETSSL_LIBRARY}
#     ${OPENSSL_CRYPTO_LIBRARY}
#     ${OPENSSL_SSL_LIBRARY}
#     glaze::glaze
#     hdr_histogram::hdr_histogram
# )

install(TARGETS app
  RUNTIME DESTINATION bin
)

# configure_file(
#     ${CMAKE_SOURCE_DIR}/get_kraken_token.py
#     ${CMAKE_BINARY_DIR}/get_kraken_token.py
#     COPYONLY
# )
