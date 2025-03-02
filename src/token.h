#pragma once

#include <string>
#include <vector>
#include <ostream>

struct token {
	enum type {
		eof,
		identifier,
		type_name,
		integral,
		floating,
		character,
		string,
		// operators
		star, slash, plus, minus, pipe_pipe, amp_amp, pipe, ampersand, hat, equal_equal, exclamation_equal, left_equal, right_equal, 
		left, right, left_left, right_right, plus_plus, minus_minus, tilde, arrow,
		// delimited
		paren_l, paren_r, bracket_l, bracket_r, brace_l, brace_r,
		// punctuation
		comma, semicolon, question, exclamation, colon, dot,
		// @=
		equals, star_equals, slash_equals, percent_equals, plus_equals, minus_equals, left_left_equals, right_right_equals, amp_equals, hat_equals, pipe_equals,
		// special words
		size_of
	};
	int line;
	int pos;
	enum type type;
	std::string text;

	token(enum type t, const std::string &str, int line, int col) : type(t), text(str), line(line), pos(col) {
	}
	static token make_char(const std::string &str, int line, int col) {
		return token(character, str.substr(1, str.length()-2), line, col);
	}
	static token make_string(const std::string &str, int line, int col) {
		return token(string, str, line, col);
	}

	bool operator==(enum type t) const { return type == t; }
	bool operator!=(enum type t) const { return type != t; }

	static std::string type_string(enum type t);

	friend std::ostream& operator<<(std::ostream &out, const token &t) {
		out << "token['" << t.text << "' " << type_string(t.type) << " " << t.line << ":" << t.pos << "]";
		return out;
	}
};


std::vector<token> lex_input(const std::string &filename);
