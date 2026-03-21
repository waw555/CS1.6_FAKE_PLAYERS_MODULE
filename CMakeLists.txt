cmake_minimum_required(VERSION 3.16)
project(cs16_fake_players_module LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(fake_players_module SHARED
    module/src/fake_players_module.cpp
)

target_include_directories(fake_players_module
    PUBLIC
        module/include
)

set_target_properties(fake_players_module PROPERTIES
    OUTPUT_NAME "fake_players_module"
)

add_executable(fake_players_module_smoke
    tests/smoke.cpp
)

target_link_libraries(fake_players_module_smoke
    PRIVATE
        fake_players_module
)

target_compile_definitions(fake_players_module_smoke
    PRIVATE
        FMP_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

install(TARGETS fake_players_module
    LIBRARY DESTINATION lib
)

install(FILES module/include/fake_players_module.h
    DESTINATION include
)

include(CTest)
add_test(NAME fake_players_module_smoke COMMAND fake_players_module_smoke)
