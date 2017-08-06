cmake_minimum_required(VERSION 3.1)
project(dhcpsnoopd)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra ${CMAKE_CXX_FLAGS}")
set(KEA_INCLUDE_DIRS "/usr/include/kea" "/usr/local/include/kea")

add_subdirectory(src)
add_subdirectory(hooks)
