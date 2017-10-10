find_library(KEADHCP kea-dhcp++)
include_directories(${KEA_INCLUDE_DIRS})

add_library(print_hook MODULE print_hook.cc)
add_library(route_hook MODULE add_routes.cc)
add_library(duid_hook MODULE detect_duid.cc)

install(TARGETS print_hook route_hook duid_hook DESTINATION ${LIB_INSTALL_DIR}/${PROJECT_NAME}/modules)
