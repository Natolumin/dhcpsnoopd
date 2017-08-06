/* dhcpsnoop is a pluggable dhcp-snooping server for linux.
 * It reads dhcp packets from nflog, and executes user-supplied actions with them
 * The user provides a library file implementing callbacks which are called on ea
 */

#include <functional>

#include "callbacks.h"
#include "config.h"
#include "nflog.h"

int main(int argc, char *argv[])
{
	auto configuration = read_configuration();

	load_callbacks(configuration.callback_path);

	auto nflog_thread = start_nflog_listener(configuration.nflog_queuenum);
	if (!nflog_thread.joinable())
		exit(EXIT_FAILURE);

	nflog_thread.join();
}
