cmake_minimum_required(VERSION 3.16)  # or 3.10, 3.20, etc.
project(blackjack C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(server
    src/server.c
    src/deck.c
    src/blackjack.c
)

add_executable(client
    src/client.c
)

