cmake_minimum_required(VERSION 3.10)
project(meimei_helper VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMODBUS REQUIRED libmodbus)

add_subdirectory(src)

add_executable(meimei_helper src/main.cpp)
target_link_libraries(meimei_helper PRIVATE meimei_common meimei_logger meimei_collect Threads::Threads)

target_include_directories(meimei_helper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/config
)
target_compile_options(meimei_helper PRIVATE -Wall -Wextra)
