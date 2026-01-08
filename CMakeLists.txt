cmake_minimum_required(VERSION 3.14)

project(GameLeaderboard)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/transaction
    ${CMAKE_SOURCE_DIR}/database
    ${CMAKE_SOURCE_DIR}/network
)

find_package(Threads REQUIRED)

find_package(PostgreSQL REQUIRED)
find_library(PQXX_LIB pqxx REQUIRED)

add_executable(TestLockMgr
    transaction/LockMgr.cpp
    TestLockMgr.cpp    
)

target_link_libraries(TestLockMgr
    Threads::Threads
)