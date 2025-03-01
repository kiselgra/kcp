#include "parser.h"
#include "tree.h"

#include <iostream>
#include <functional>
#include <cassert>

using namespace std;
using namespace ast;

#define helper(X,...) auto X = [&](__VA_ARGS__)
#define rule(X) auto X = [&]()
#define rrule(X) X = [&]()

struct parser {

};

void parse(const vector<token> &tokens) {

	int current = 0;

	helper(at_end) {
		return tokens[current] == token::eof;
	};
	helper(previous) {
		assert(current > 0);
		return tokens[current-1];
	};
	helper(advance) {
		if (!at_end()) current++;
		return previous();
	};
	helper(peek) -> const token& {
		return tokens[current];
	};
	helper(check, enum token::type t) {
		if (at_end()) return false;
		return peek() == t;
	};
	helper(consume, enum token::type type, const std::string &message) {
		if (check(type)) return advance();
		throw parse_error(peek(), message);
	};
	helper(match, auto... ts) {
		using expand = enum token::type[];	// c++ is crazy...
		for (auto x : expand{ts...})
			if (check(x)) {
				advance();
				return true;
			}
		return false;
	};
	
	std::function<pointer_to<ast::expression>()> expression;

	rule(primary) -> pointer_to<ast::expression> {
		if (match(token::number))
			return make_node<ast::number>(previous());
		else if (match(token::paren_l)) {
			auto exp = expression();
			consume(token::paren_r, "Expect ')' after expression.");
			return exp;
		}
		else
			throw parse_error(peek(), "Expect expression.");
 	};
	
	// expressions with multiple operands, i.e. arithemtic, logical, etc
	auto parse_nary = [&]<typename node_t>(auto &subrule, auto... t) {
		pointer_to<node_t> outer = nullptr;
		auto lhs = subrule();
		while (match(t...)) {
			token op = previous();
			auto rhs = subrule();
			if (!outer)
				outer = make_node<node_t>(op, lhs, rhs);
			else
				outer->add(op, rhs);
		}
		return outer ? outer : lhs;
	};
	rule(factor) {
		return parse_nary.template operator()<ast::arith>(primary, token::star, token::slash);
	};
	rule(term) {
		return parse_nary.template operator()<ast::arith>(factor, token::plus, token::minus);
	};
	std::function<pointer_to<ast::expression>()> conditional_exp;
	rrule(conditional_exp) -> pointer_to<ast::expression> {
		auto exp = term();
		if (match(token::question)) {
			token q = previous();
			auto consequent = expression();
			auto c = consume(token::colon, "Expect ':' following '?'-subexpression.");
			auto alternative = conditional_exp();
			auto cond = make_node<ast::conditional>(exp, q, consequent, c, alternative);
			return cond;
		}
		return exp;
	};
	rule(assignment_exp) {
		return parse_nary.template operator()<ast::assign>(conditional_exp, 
														   token::equals, token::star_equals, token::slash_equals, token::percent_equals, token::plus_equals, token::minus_equals, 
														   token::left_left_equals, token::right_right_equals, token::amp_equals, token::hat_equals, token::pipe_equals);
	};
	rrule(expression) {
		return parse_nary.template operator()<sequence>(assignment_exp, token::comma);
	};

	auto x = expression();
	ast::printer p(std::cout);
	x->traverse_with(&p);
}

// vim: filetype=cpp-parse
