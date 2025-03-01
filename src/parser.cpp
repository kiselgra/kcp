#include "parser.h"
#include "tree.h"

#include <iostream>
#include <functional>
#include <cassert>

using namespace std;
using namespace ast;

#define helper(X,...) auto X = [&](__VA_ARGS__)
#define rule(X) auto X = [&]()
#define frule(X) X = [&]()
#define rrule(X,Y) std::function<pointer_to<ast::Y>()> X = [&]() -> pointer_to<ast::Y>

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
	std::function<pointer_to<ast::expression>()> cast_exp;
	std::function<pointer_to<ast::expression>()> assignment_exp;

	rule(type_name) {
		return make_node<number>(token(token::number, "NOT-IMPLEMENTED", -1, -1));
	};
	rule(identifier) {
		return make_node<number>(token(token::number, "NOT-IMPLEMENTED", -1, -1));
	};

	rule(primary_exp) -> pointer_to<ast::expression> {
		if (match(token::number))
			return make_node<ast::number>(previous());
		else if (match(token::paren_l)) {
			auto exp = expression();
			consume(token::paren_r, "Expect ')' after expression.");
			return exp;
		}
		// TODO
		else
			throw parse_error(peek(), "Expect expression.");
 	};
	rule(postfix_exp) -> pointer_to<ast::expression> {
		auto exp = primary_exp();
		if (match(token::paren_l)) {
			auto opening = previous();
			auto call = make_node<ast::call>(opening, exp);
			while (match(token::comma)) {
				auto next_arg = assignment_exp();
				call->add(next_arg);
			}
			consume(token::paren_r, "Expect ')' at end of call.");
			return call;
		}
		else if (match(token::bracket_l)) {
			auto opening = previous();
			auto subscript = expression();
			consume(token::bracket_r, "Expect ']' after subscript.");
			return make_node<ast::subscript>(opening, exp, subscript);
		}
		else if (match(token::dot, token::arrow)) {
			auto accessor = previous();
			auto inner = identifier();
			return make_node<ast::member_access>(accessor, exp, inner);
		}
		else if (match(token::plus_plus, token::minus_minus)) {
			auto op = previous();
			return make_node<postfix>(op, exp);
		}
		return primary_exp();
	};
	rrule(unary_exp, expression) {
		if (match(token::plus_plus, token::minus_minus)) {
			auto op = previous();
			auto sub = unary_exp();
			return make_node<prefix>(op, sub);
		}
		else if (match(token::ampersand, token::star, token::plus, token::minus, token::tilde, token::exclamation)) {
			auto op = previous();
			auto sub = cast_exp();
			return make_node<unary>(op, sub);
		}
		else if (match(token::size_of)) {
			auto sizeof_token = previous();
			if (match(token::paren_l)) {
				auto type = type_name();
				consume(token::paren_r, "Expect ')' after type name.");
				return make_node<unary>(sizeof_token, type);
			}
			else {
				auto sub = unary_exp();
				return make_node<unary>(sizeof_token, sub);
			}
		}
		return postfix_exp();
	};
	frule(cast_exp) -> pointer_to<ast::expression> {
		if (match(token::paren_l)) {
			auto type = type_name();
			auto closing = consume(token::paren_r, "Expect ')' after cast target type.");
			auto subexp = cast_exp();
			return make_node<cast>(closing, type, subexp);
		}
		return unary_exp();
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
	rule(multiplicative_exp) {
		return parse_nary.template operator()<ast::arith>(cast_exp, token::star, token::slash);
	};
	rule(additive_exp) {
		return parse_nary.template operator()<ast::arith>(multiplicative_exp, token::plus, token::minus);
	};
	rule(shift_exp) {
		return parse_nary.template operator()<ast::bitwise>(additive_exp, token::left_left, token::right_right);
	};
	rule(relational_exp) {
		return parse_nary.template operator()<ast::relational>(shift_exp, token::left, token::right, token::left_equal, token::right_equal);
	};
	rule(equality_exp) {
		return parse_nary.template operator()<ast::equality>(relational_exp, token::equal_equal, token::exclamation_equal);
	};
	rule(binary_xor_exp) {
		return parse_nary.template operator()<ast::bitwise>(equality_exp, token::hat);
	};
	rule(binary_and_exp) {
		return parse_nary.template operator()<ast::bitwise>(binary_xor_exp, token::ampersand);
	};
	rule(binary_or_exp) {
		return parse_nary.template operator()<ast::bitwise>(binary_and_exp, token::pipe);
	};
	rule(logical_and_exp) {
		return parse_nary.template operator()<ast::logical>(binary_or_exp, token::amp_amp);
	};
	rule(logical_or_exp) {
		return parse_nary.template operator()<ast::logical>(logical_and_exp, token::pipe_pipe);
	};
	rrule(conditional_exp,expression) {
		auto exp = logical_or_exp();
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
	frule(assignment_exp) {
		return parse_nary.template operator()<ast::assign>(conditional_exp, 
														   token::equals, token::star_equals, token::slash_equals, token::percent_equals, token::plus_equals, token::minus_equals, 
														   token::left_left_equals, token::right_right_equals, token::amp_equals, token::hat_equals, token::pipe_equals);
	};
	frule(expression) {
		return parse_nary.template operator()<sequence>(assignment_exp, token::comma);
	};

	auto x = expression();
	ast::printer p(std::cout);
	x->traverse_with(&p);
}

// vim: filetype=cpp-parse
