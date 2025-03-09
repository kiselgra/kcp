#pragma once

#include <string>
#include <vector>
#include <ostream>
#include <sstream>

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
		star, slash, plus, minus, pipe_pipe, amp_amp, pipe, percent, ampersand, hat, equal_equal, exclamation_equal, left_equal, right_equal, 
		left, right, left_left, right_right, plus_plus, minus_minus, tilde, arrow,
		// delimited
		paren_l, paren_r, bracket_l, bracket_r, brace_l, brace_r,
		// punctuation
		comma, semicolon, question, exclamation, colon, dot, ellipsis,
		// @=
		equals, star_equals, slash_equals, percent_equals, plus_equals, minus_equals, left_left_equals, right_right_equals, amp_equals, hat_equals, pipe_equals,
		// special words for declarations
		size_of, kw_char, kw_const, kw_double, kw_enum, kw_float, kw_int, kw_long, kw_short, kw_signed, kw_struct, kw_union, kw_unsigned, kw_void, kw_volatile,
		kw_static, kw_auto, kw_extern, kw_register, kw_typedef,
		// special words for statements
		kw_if, kw_break, kw_case, kw_continue, kw_default, kw_do, kw_else, kw_for, kw_goto, kw_return, kw_switch, kw_while,
		attribute, // the entire attribute block
		// call setline('.', join(sort(split(getline('.'), ' ')), " "))
	};
	int line;
	int pos;
	enum type type;
	std::string text;
	std::string file;

	token(enum type t, const std::string &str, int line, int col, const std::string &file) : type(t), text(str), line(line), pos(col), file(file) {
	}
	static token make_char(const std::string &str, int line, int col, const std::string &file) {
		return token(character, str.substr(1, str.length()-2), line, col, file);
	}
	static token make_string(const std::string &str, int line, int col, const std::string &file) {
		return token(string, str, line, col, file);
	}
	static token make_attribute(const std::string &str, int line, int col, const std::string &file) {
		return token(attribute, str, line, col, file);
	}

	bool operator==(enum type t) const { return type == t; }
	bool operator!=(enum type t) const { return type != t; }

	static std::string type_string(enum type t);

	friend std::ostream& operator<<(std::ostream &out, const token &t) {
		out << "token['" << t.text << "' " << type_string(t.type) << " " << t.file << ":" << t.line << "," << t.pos << "]";
		return out;
	}
};

struct lexer_error : public std::runtime_error {
	int line, col;
	std::string lexeme;
	std::string full;
	lexer_error(int line, int col, const std::string lexeme, const std::string &message) : runtime_error(message), line(line), col(col), lexeme(lexeme) {
		std::ostringstream oss;
		oss << "Lexer Error: " << message << " @" << line << ":" << col << ", got input '" << lexeme << "'";
		full = oss.str();
	}
	const char* what() const noexcept override {	// order noexcept/override matters to gcc 14.2.1
		return full.c_str();
	}
};


std::vector<token> lex_input(const std::string &filename);
