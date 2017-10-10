#include "hooks.h"

#include <iostream>
#include <kea/dhcp/dhcp6.h>
#include <kea/dhcp/pkt6.h>

extern "C" {
int is_concurrent_safe = false;
void dhcp_callback(std::unique_ptr<isc::dhcp::Pkt6> packet)
{
	// Ignore relayed packets, as they would have the mac address of the next/previous relay
	// instead of the end-host
	auto type = packet->getType();
	if (type == DHCPV6_RELAY_FORW || type == DHCPV6_RELAY_REPL) {
		std::cerr << "Ignoring relayed packet" << std::endl;
		return;
	}
	if (!packet->getRemoteHWAddr()) {
		std::cerr << "No HW addr information found" << std::endl;
		return;
	}
	std::cout << packet->getClientId()->toText() << " "
	          << packet->getRemoteHWAddr()->toText(false) << std::endl;
}
}
