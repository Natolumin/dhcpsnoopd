#pragma once

#include <kea/dhcp/pkt6.h>

extern "C" {
extern int is_concurrent_safe;
void dhcp_callback(std::unique_ptr<isc::dhcp::Pkt6>);
}
