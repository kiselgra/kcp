#pragma once

#include <string>
#include <vector>

struct token {
	enum type {
		eof,
		identifier,
		number,
	};
	int line;
	int pos;
	enum type type;
	std::string text;

	token(enum type t, const char *str, int line) : type(t), text(str), line(line) {
	}

	static std::string type_name(enum type t);
};


std::vector<token> lex_input(const std::string &filename);
