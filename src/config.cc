#include "config.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct configuration read_configuration(const std::vector<std::string> &args)
{
	if (args.size() < 3 || std::find(args.cbegin() + 1, args.cend(), "-h") != args.cend() ||
	    std::find(args.cbegin() + 1, args.cend(), "--help") != args.cend()) {
		std::cout << "Usage: " << args[0] << " NFLOG_GROUP HOOK.so" << std::endl;
		exit(EXIT_FAILURE);
	}
	return {std::stoi(args[1]), args[2]};
}
