/* NF_LOG related functions, */

#include "nflog.h"

#include <arpa/inet.h>
#include <iostream>
#include <libmnl/libmnl.h>
#include <linux/netfilter.h>
#include <linux/netfilter/nfnetlink_log.h>
#include <pthread.h>
#include <stdexcept>
#include <thread>

#include "callbacks.h"

/*
 * The following code is included from the examples/netfilter folder distributed with nf_log, and is
 * licensed under the
 * GPL2 as a result
 */

static struct nlmsghdr *nflog_build_cfg_request(char *buf, uint8_t command, int qnum)
{
	struct nlmsghdr *nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type = (NFNL_SUBSYS_ULOG << 8) | NFULNL_MSG_CONFIG;
	nlh->nlmsg_flags = NLM_F_REQUEST;

	struct nfgenmsg *nfg =
	    static_cast<struct nfgenmsg *>(mnl_nlmsg_put_extra_header(nlh, sizeof(*nfg)));
	nfg->nfgen_family = AF_INET;
	nfg->version = NFNETLINK_V0;
	nfg->res_id = htons(qnum);

	struct nfulnl_msg_config_cmd cmd = {
	    .command = command,
	};
	mnl_attr_put(nlh, NFULA_CFG_CMD, sizeof(cmd), &cmd);

	return nlh;
}

static struct nlmsghdr *nflog_build_cfg_params(char *buf, uint8_t mode, int range, int qnum)
{
	struct nlmsghdr *nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type = (NFNL_SUBSYS_ULOG << 8) | NFULNL_MSG_CONFIG;
	nlh->nlmsg_flags = NLM_F_REQUEST;

	struct nfgenmsg *nfg =
	    static_cast<struct nfgenmsg *>(mnl_nlmsg_put_extra_header(nlh, sizeof(*nfg)));
	nfg->nfgen_family = AF_UNSPEC;
	nfg->version = NFNETLINK_V0;
	nfg->res_id = htons(qnum);

	struct nfulnl_msg_config_mode params = {
	    .copy_range = htonl(range), .copy_mode = mode,
	};
	mnl_attr_put(nlh, NFULA_CFG_MODE, sizeof(params), &params);

	return nlh;
}

static struct mnl_socket *setup_mnl_socket(uint16_t qnum)
{
	char buf[MNL_SOCKET_BUFFER_SIZE];
	auto *socket = mnl_socket_open(NETLINK_NETFILTER);
	if (!socket) {
		std::perror("Could not open mnl socket: ");
		return nullptr;
	}

	if (mnl_socket_bind(socket, 0, MNL_SOCKET_AUTOPID) < 0) {
		std::perror("Could not bind to MNL socket: ");
		return nullptr;
	}

	// bind our listener to the requested nflog queue
	auto header = nflog_build_cfg_request(buf, NFULNL_CFG_CMD_BIND, qnum);
	if (mnl_socket_sendto(socket, header, header->nlmsg_len) < 0) {
		std::perror("Could not bind to the queue");
		return nullptr;
	}

	// Configure the mnl socket to copy the full packet data (0 means the maximum range)
	header = nflog_build_cfg_params(buf, NFULNL_COPY_PACKET, 0, qnum);
	if (mnl_socket_sendto(socket, header, header->nlmsg_len) < 0) {
		std::perror("Could not configure the packet copy mode");
		return nullptr;
	}

	return socket;
}

int nflog_loop(const struct mnl_socket *socket)
{
	uint8_t *buf = new uint8_t[MNL_SOCKET_BUFFER_SIZE];

	auto portid = mnl_socket_get_portid(socket);
	int ret;
	ssize_t numbytes;

	do {
		numbytes = mnl_socket_recvfrom(socket, buf, MNL_SOCKET_BUFFER_SIZE);
		if (numbytes < 0) {
			std::perror("Could not receive data from nflog");
			throw std::runtime_error("Error receiving from nflog");
		}

		ret = mnl_cb_run(buf, numbytes, 0, portid, callback_shim, NULL);
		if (ret < 0) {
			std::cerr << "Failed callback" << std::endl;
			continue;
		}
	} while (1);
}

std::thread start_nflog_listener(int queue_number)
{
	auto socket = setup_mnl_socket(queue_number);

	if (!socket) {
		return std::thread();
	}

	std::thread nflog_thread(nflog_loop, socket);

#if defined(__GLIBCXX__) || (defined(_LIBCPP_VERSION) && defined(_LIBCPP_HAS_THREAD_API_PTHREAD))
	// Setup a pretty name if we're using pthreads underneath
	auto handle = nflog_thread.native_handle();
	pthread_setname_np(handle, "nflog listener");
#endif

	return nflog_thread;
}
