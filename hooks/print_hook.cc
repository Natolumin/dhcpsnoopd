#include "hooks.h"

#include <iostream>
#include <kea/dhcp/pkt6.h>

extern "C" {
int is_concurrent_safe = false;
void dhcp_callback(std::unique_ptr<isc::dhcp::Pkt6> packet) { std::cout << packet->toText(); }
}
