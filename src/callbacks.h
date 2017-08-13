/* Callback-related functions, for loading and accessing the user-provided callbacks */
#pragma once

#include <functional>

#include <kea/dhcp/pkt6.h>
#include <libmnl/libmnl.h>

typedef std::unique_ptr<isc::dhcp::Pkt6> Pkt6Uptr;

extern bool cb_is_concurrent;
extern std::function<void(Pkt6Uptr)> dhcp_callback;

void load_callbacks(std::string library_path);
int callback_shim(const struct nlmsghdr *header, void *data);
