find_library(KEADHCP kea-dhcp++)
include_directories(${KEA_INCLUDE_DIRS})

add_library(print_hook MODULE print_hook.cc)
