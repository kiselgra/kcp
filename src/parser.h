#pragma once

#include "token.h"

#include <vector>
#include <stdexcept>

#include <sstream>

struct parse_error : public std::runtime_error {
	token at;
	std::string full;
	parse_error(token at, const std::string &message) : runtime_error(message), at(at) {
		std::ostringstream oss;
		oss << "Parse Error:" << message << " @" << at.line << ":" << at.pos;
		full = oss.str();
	}
	const char* what() const noexcept override {	// order noexcept/override matters to gcc 14.2.1
		return full.c_str();
	}
};

void parse(const std::vector<token> &tokens);


