#pragma once

#include <string>
#include <vector>

struct configuration {
	int nflog_queuenum;
	std::string callback_path;
};

struct configuration read_configuration(const std::vector<std::string> &);
