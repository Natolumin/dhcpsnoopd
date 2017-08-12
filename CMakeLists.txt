cmake_minimum_required(VERSION 2.6)
project(dhcpsnoopd)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra ${CMAKE_CXX_FLAGS}")
set(KEA_INCLUDE_DIRS "/usr/include/kea" "/usr/local/include/kea" CACHE PATH "Location of KEA include files")
set(LIB_INSTALL_DIR "lib/" CACHE STRING "Location where the modules will be installed")

add_subdirectory(src)
add_subdirectory(hooks)
