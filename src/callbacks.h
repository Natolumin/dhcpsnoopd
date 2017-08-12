/* Callback-related functions, for loading and accessing the user-provided callbacks */
#pragma once

#include <functional>

// We don't need this here, but we need it to be included before netinet/in.h to define __USE_KERNEL_IPV6_DEFS
#include <linux/ipv6.h>

#include <kea/dhcp/pkt6.h>

typedef std::unique_ptr<isc::dhcp::Pkt6> Pkt6Uptr;

extern bool cb_is_concurrent;
extern std::function<void(Pkt6Uptr)> dhcp_callback;

void load_callbacks(std::string library_path);
int callback_shim(const struct nlmsghdr *header, void *data);
