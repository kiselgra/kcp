#pragma once

#include <string>
#include <vector>

struct token {
	enum type {
		eof,
		identifier,
		number,
		// operators
		star, slash, plus, minus,
		// delimited
		paren_l, paren_r, bracket_l, bracket_r, brace_l, brace_r,
		// punctuation
		comma, semicolon, question, exclamation, colon,
		// @=
		equals, star_equals, slash_equals, percent_equals, plus_equals, minus_equals, left_left_equals, right_right_equals, amp_equals, hat_equals, pipe_equals,
	};
	int line;
	int pos;
	enum type type;
	std::string text;

	token(enum type t, const char *str, int line, int col) : type(t), text(str), line(line), pos(col) {
	}

	bool operator==(enum type t) const { return type == t; }
	bool operator!=(enum type t) const { return type != t; }

	static std::string type_name(enum type t);
};


std::vector<token> lex_input(const std::string &filename);
