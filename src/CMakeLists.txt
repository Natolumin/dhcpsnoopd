add_executable(${PROJECT_NAME} main.cc nflog.cc callbacks.cc config.cc)

find_library(KEADHCP kea-dhcp++)
find_library(KEAASIOLINK kea-asiolink)
include_directories(${KEA_INCLUDE_DIRS})

include(FindPkgConfig)
pkg_search_module(LIBMNL libmnl)
if (NOT LIBMNL_FOUND)
    message(FATAL_ERROR "libmnl not found")
endif(NOT LIBMNL_FOUND)
include_directories(${LIBMNL_INCLUDE_DIRS})

find_package(Threads)

find_package(Boost 1.44.0 REQUIRED COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
    ${LIBMNL_LIBRARIES} ${KEADHCP} ${KEAASIOLINK}
    ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS}
    ${Boost_SYSTEM_LIBRARY}
)

install(TARGETS ${PROJECT_NAME} DESTINATION "bin")
