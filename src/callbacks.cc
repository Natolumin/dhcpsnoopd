#include "callbacks.h"

// This needs to be first to define __USE_KERNEL_IPV6_DEFS
#include <linux/ipv6.h>

#include <arpa/inet.h>
#include <dlfcn.h>
#include <functional>
#include <iostream>
#include <kea/asiolink/io_address.h>
#include <kea/dhcp/pkt6.h>
#include <libmnl/libmnl.h>
#include <linux/netfilter/nfnetlink_log.h>
#include <linux/udp.h>
#include <net/if.h>

bool cb_is_concurrent;
std::function<void(Pkt6Uptr)> dhcp_callback;

void load_callbacks(const std::string library_path)
{
	auto libhandle = dlopen(library_path.c_str(), RTLD_NOW);
	char *err;
	if ((err = dlerror())) {
		std::cerr << "Error while opening hook library" << err;
		exit(EXIT_FAILURE);
	}

	cb_is_concurrent = *(static_cast<int *>(dlsym(libhandle, "is_concurrent_safe")));
	if ((err = dlerror())) {
		std::cerr << "is_concurrent_safe is not defined in the hook library";
		exit(EXIT_FAILURE);
	}

	dhcp_callback = reinterpret_cast<void (*)(Pkt6Uptr)>(dlsym(libhandle, "dhcp_callback"));
	if ((err = dlerror())) {
		std::cerr << "dhcp_callback is not defined in the hook library" << err;
		exit(EXIT_FAILURE);
	}
}

// TODO: Add hwaddr to this struct
struct pinfo {
	uint32_t ifindex;
	uint16_t pl_len;
	uint8_t *payload;
};

static int parse_payload(const struct nlattr *attr, void *data)
{
	uint8_t *pl_ptr;
	int type = mnl_attr_get_type(attr);
	auto *lv = static_cast<struct pinfo *>(data);

	switch (type) {
	// If we have an OUTDEV, that means we're a server. If we have an INDEV we're a client. We
	// care more about the server usecase, so we won't override OUTDEV with INDEV. It shouldn't
	// happen though, as that would mean we're forwarding a dhcp packet which is /just wrong/
	case NFULA_IFINDEX_INDEV:
		if (lv->ifindex)
			break;
	case NFULA_IFINDEX_OUTDEV:
		if (mnl_attr_validate(attr, MNL_TYPE_U32) < 0) {
			return MNL_CB_ERROR;
		}
		lv->ifindex = ntohl(mnl_attr_get_u32(attr));
		break;

	case NFULA_PAYLOAD:
		lv->pl_len = mnl_attr_get_payload_len(attr);
		if (!lv->pl_len)
			return MNL_CB_ERROR;

		pl_ptr = static_cast<uint8_t *>(mnl_attr_get_payload(attr));
		try {
			// ipv6hdr needs 4-byte alignment
			size_t size = lv->pl_len + 4;
			void *buf = new uint8_t[size];
			lv->payload = static_cast<uint8_t *>(std::align(4, lv->pl_len, buf, size));
		} catch (std::bad_alloc e) {
			std::cerr << e.what() << std::endl;
			return MNL_CB_ERROR;
		}

		std::copy(pl_ptr, pl_ptr + lv->pl_len, lv->payload);
		break;
	}

	return MNL_CB_OK;
}

int callback_shim(const struct nlmsghdr *header, __attribute__((unused)) void *data)
{
	assert(dhcp_callback);

	struct pinfo pinfo = {0, 0, nullptr};

	if (mnl_attr_parse(header, 0, parse_payload, &pinfo) <= MNL_CB_ERROR) {
		return MNL_CB_ERROR;
	}

	if (!pinfo.pl_len) {
		std::cerr << "There was no payload in the packet" << std::endl;
		return MNL_CB_ERROR;
	}

	/* Unpack the IP + UDP header */

	auto offset = sizeof(struct ipv6hdr) + sizeof(struct udphdr);
	if (pinfo.pl_len < offset)
		return MNL_CB_ERROR;

	struct ipv6hdr *ip_hdr = (struct ipv6hdr *)(pinfo.payload);
	if (ip_hdr->nexthdr != IPPROTO_UDP) {
		return MNL_CB_ERROR;
	}
	struct udphdr *udp_hdr = (struct udphdr *)(pinfo.payload + sizeof(struct ipv6hdr));

	try {
		auto packet = std::make_unique<isc::dhcp::Pkt6>(pinfo.payload + offset,
		                                                pinfo.pl_len - offset,
		                                                isc::dhcp::Pkt6::DHCPv6Proto::UDP);
		packet->unpack();

		// Remote/Local here for an output hook. Invert for an output hook
		packet->setLocalAddr(
		    isc::asiolink::IOAddress::fromBytes(AF_INET6, (uint8_t *)&(ip_hdr->saddr)));
		packet->setRemoteAddr(
		    isc::asiolink::IOAddress::fromBytes(AF_INET6, (uint8_t *)&(ip_hdr->daddr)));
		packet->setRemotePort(ntohs(udp_hdr->source));
		packet->setLocalPort(ntohs(udp_hdr->dest));
		packet->setIndex(pinfo.ifindex);
		{
			char ifname[IF_NAMESIZE];
			auto ret = if_indextoname(pinfo.ifindex, ifname);
			if (ret) {
				packet->setIface(ret);
			}
		}

		if (!cb_is_concurrent) {
			dhcp_callback(std::move(packet));
		} else {
			std::cerr << "Concurrent callbacks aren't supported yet";
			exit(EXIT_FAILURE);
		}
	} catch (std::exception e) {
		std::cerr << e.what() << std::endl;
		return MNL_CB_ERROR;
	}
	return MNL_CB_OK;
}
