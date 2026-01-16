cmake_minimum_required(VERSION 3.14)

project(GameLeaderboard)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/transaction
    ${CMAKE_SOURCE_DIR}/database
    ${CMAKE_SOURCE_DIR}/network
)

find_package(Threads REQUIRED)
find_package(PostgreSQL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

file(GLOB_RECURSE SOURCES 
    "transaction/*.cpp"
    "database/*.cpp"
    "network/*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES} GameLeaderboard.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${PQXX_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${PQXX_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME}
    Threads::Threads
    pqxx
    pq
)

add_executable(TransactionServer ${SOURCES} ServerMain.cpp)
target_include_directories(TransactionServer PRIVATE ${PQXX_INCLUDE_DIRS})
target_link_directories(TransactionServer PRIVATE ${PQXX_LIBRARY_DIRS})
target_link_libraries(TransactionServer
    Threads::Threads
    pqxx
    pq
)