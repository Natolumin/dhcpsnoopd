find_library(KEADHCP kea-dhcp++)
include_directories(${KEA_INCLUDE_DIRS})

add_library(print_hook MODULE print_hook.cc)
add_library(route_hook MODULE add_routes.cc)
