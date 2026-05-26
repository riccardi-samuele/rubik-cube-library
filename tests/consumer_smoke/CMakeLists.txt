cmake_minimum_required(VERSION 3.20)

project(rubik_consumer_smoke LANGUAGES CXX)

find_package(rubik CONFIG REQUIRED)

add_executable(rubik_consumer_smoke main.cpp)
target_link_libraries(rubik_consumer_smoke PRIVATE rubik::rubik)
