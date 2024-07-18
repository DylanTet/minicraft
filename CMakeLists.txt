cmake_minimum_required(VERSION 3.8)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(MiniCraft)

add_executable(main main.cpp)
target_include_directories(main PRIVATE "{CMAKE_SOURCE_DIR}/../../../../../opt/homebrew/include/asio.hpp" "{CMAKE_SOURCE_DIR}/../../../../../opt/homebrew/opt/ncurses/include/ncurses.h")


target_compile_features(main PUBLIC cxx_std_17)
