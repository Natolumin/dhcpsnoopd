#pragma once

#include <string>

struct configuration {
	int nflog_queuenum;
	std::string callback_path;
};

struct configuration read_configuration();
