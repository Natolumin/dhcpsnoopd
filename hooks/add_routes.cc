#include "hooks.h"

#include <iostream>
#include <kea/dhcp/dhcp6.h>
#include <kea/dhcp/option6_iaprefix.h>
#include <kea/dhcp/pkt6.h>
#include <libmnl/libmnl.h>
#include <linux/rtnetlink.h>

const uint8_t TABLE_NUMBER = 47;

static mnl_socket *mnl_socket;
static char *buf;

static int inject_route(const struct in6_addr *prefix, const struct in6_addr *nexthop,
			uint8_t prefix_len, uint32_t lifetime, uint32_t ifindex);

static inline int copy_addr(isc::asiolink::IOAddress src, struct in6_addr *dst);

extern "C" {
int is_concurrent_safe = false; // static buffer and static socket
void dhcp_callback(std::unique_ptr<isc::dhcp::Pkt6> packet)
{
	auto type = packet->getType();
	if (!(type == DHCPV6_REPLY)) {
		std::cerr << "Not a reply packet, ignoring" << std::endl;
		return;
	}

	// Whatever the options, the nexthop will be the destination lladdr for the packet (+ the
	// corresponding interface), which we get from the netlink packet and set in the Pkt6 struct
	auto ifindex = packet->getIndex();
	struct in6_addr nexthop;
	if (copy_addr(packet->getRemoteAddr(), &nexthop) < 0) {
		std::cerr << "Invalid remote address" << std::endl;
		return;
	}

	// We assume only one IA_PD with one IA_PREFIX, because I'm just lazy and kea's
	// OptionCollection is ergh
	auto iapd = packet->getOption(D6O_IA_PD);
	if (!iapd) {
		std::cerr << "No IA_PD, ignoring" << std::endl;
		return;
	}

	// IA_PREFIX inside of IA_PD will hold the prefix_len, valid_lft and prefix values
	auto odata = iapd->getOption(D6O_IAPREFIX)->toBinary();
	if (odata.empty()) {
		std::cerr << "No IA_PREFIX, ignoring" << std::endl;
		return;
	}
	auto iaprefix = isc::dhcp::Option6IAPrefix(D6O_IAPREFIX, odata.begin(), odata.end());

	uint32_t lifetime = iaprefix.getValid();
	uint8_t prefix_len = iaprefix.getLength();

	struct in6_addr prefix;
	if (copy_addr(iaprefix.getAddress(), &prefix) < 0) {
		std::cerr << "Invalid given prefix" << std::endl;
		return;
	}

	std::cout << iaprefix.getAddress().toText() << "/" << (int32_t)iaprefix.getLength() << " "
		  << "via " << packet->getRemoteAddr().toText() << " dev " << packet->getIface()
		  << " proto dhcp metric 1024 expires " << iaprefix.getValid() << "sec pref medium"
		  << std::endl;

	if (inject_route(&prefix, &nexthop, prefix_len, lifetime, ifindex) < 0) {
		std::cerr << "Could not inject route :( " << std::endl;
		return;
	}
}
}

static inline int copy_addr(isc::asiolink::IOAddress src, struct in6_addr *dst)
{
	auto nh_addr = src.toBytes();
	if (nh_addr.size() < 16) {
		return -1;
	}
	std::copy(nh_addr.cbegin(), nh_addr.cend(), reinterpret_cast<uint8_t *>(dst));
	return 0;
}

__attribute__((constructor)) static void setup_rtnl_socket()
{
	mnl_socket = mnl_socket_open(NETLINK_ROUTE);
	if (!mnl_socket) {
		perror("mnl_socket_open");
		exit(EXIT_FAILURE);
	}
	if (mnl_socket_bind(mnl_socket, 0, MNL_SOCKET_AUTOPID) < 0) {
		perror("mnl_socket_bind");
		exit(EXIT_FAILURE);
	}

	buf = new char[MNL_SOCKET_BUFFER_SIZE];
	if (!buf) {
		perror("Could not allocate memory");
		exit(EXIT_FAILURE);
	}
}

static int inject_route(const struct in6_addr *prefix, const struct in6_addr *nexthop,
			uint8_t prefix_len, uint32_t lifetime, uint32_t ifindex)
{
	auto handle = mnl_nlmsg_put_header(buf);
	handle->nlmsg_type = RTM_NEWROUTE;
	handle->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE;
	auto seq = handle->nlmsg_seq = time(NULL);

	struct rtmsg *rtm =
	    (struct rtmsg *)mnl_nlmsg_put_extra_header(handle, sizeof(struct rtmsg));
	rtm->rtm_family = AF_INET6;
	rtm->rtm_dst_len = prefix_len;
	rtm->rtm_src_len = 0;
	rtm->rtm_tos = 0;

	rtm->rtm_table = TABLE_NUMBER;
	rtm->rtm_protocol = RTPROT_DHCP;
	rtm->rtm_scope = RT_SCOPE_SITE;
	rtm->rtm_type = RTN_UNICAST;

	rtm->rtm_flags = 0;

	mnl_attr_put(handle, RTA_DST, sizeof(*nexthop), prefix);
	mnl_attr_put(handle, RTA_GATEWAY, sizeof(*nexthop), nexthop);
	mnl_attr_put_u32(handle, RTA_OIF, ifindex);
	mnl_attr_put_u32(handle, RTA_EXPIRES, lifetime);

	auto portid = mnl_socket_get_portid(mnl_socket);

	if (mnl_socket_sendto(mnl_socket, handle, handle->nlmsg_len) < 0) {
		perror("mnl_socket_sendto");
		return -1;
	}

	auto bytes_received = mnl_socket_recvfrom(mnl_socket, buf, MNL_SOCKET_BUFFER_SIZE);
	if (bytes_received < 0) {
		perror("mnl_socket_recvfrom");
		return -1;
	}

	auto ret = mnl_cb_run(buf, bytes_received, seq, portid, NULL, NULL);
	// If the same route exists, the expiry will be reset, even if it returns an error
	if (ret < 0 && errno != EEXIST) {
		perror("mnl_cb_run");
		return -1;
	}

	return 0;
}
