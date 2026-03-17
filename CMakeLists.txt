cmake_minimum_required(VERSION 3.16)
project(cs16_fake_players_module LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(fake_players_module SHARED
    module/src/fake_players_module.cpp
)

target_include_directories(fake_players_module
    PRIVATE
        module/include
)

set_target_properties(fake_players_module PROPERTIES
    OUTPUT_NAME "fake_players_module"
)
