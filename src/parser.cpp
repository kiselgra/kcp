#include "parser.h"
#include "tree.h"

#include <iostream>
#include <functional>
#include <cassert>

using namespace std;
using namespace ast;

#define helper(X,...) auto X = [&](__VA_ARGS__)
#define rule(X) auto X = [&]()
#define prule(X,P) auto X = [&](P)
#define frule(X) X = [&]()
#define rrule(X,Y) std::function<pointer_to<ast::Y>()> X = [&]() -> pointer_to<ast::Y>
#define prrule(X,P,Y) std::function<pointer_to<ast::Y>(P)> X = [&](P) -> pointer_to<ast::Y>

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
	helper(peek1) -> const token& {
		if (current < tokens.size())
			return tokens[current];
		return tokens[current]; // will be eof
	};
	helper(check1, enum token::type t) {
		return peek1() == t;
	};
	
	std::function<pointer_to<ast::expression>()> expression;
	std::function<pointer_to<ast::expression>()> cast_exp;
	std::function<pointer_to<ast::expression>()> assignment_exp;

	rule(type_name) {
		auto tok = consume(token::identifier, "Expect identifier (for type name).");
		return make_node<ast::identifier>(tok);
	};
	rule(identifier) {
		auto tok = consume(token::identifier, "Expect identifier.");
		return make_node<ast::identifier>(tok);
	};

	/* 
	 * Expressions.
	 *
	 */

	rule(primary_exp) -> pointer_to<ast::expression> {
		if (match(token::identifier))
			return make_node<ast::identifier>(previous());
		else if (match(token::integral))
			return make_node<ast::integral_lit>(previous());
		else if (match(token::floating))
			return make_node<ast::float_lit>(previous());
		else if (match(token::character))
			return make_node<ast::character_lit>(previous());
		else if (match(token::string))
			return make_node<ast::string_lit>(previous());
		else if (match(token::paren_l)) {
			auto exp = expression();
			consume(token::paren_r, "Expect ')' after expression.");
			return exp;
		}
		else
			throw parse_error(peek(), "Expect expression.");
 	};
	rule(postfix_exp) -> pointer_to<ast::expression> {
		auto exp = primary_exp();
		if (match(token::paren_l)) {
			auto opening = previous();
			auto call = make_node<ast::call>(opening, exp);
			if (!check(token::paren_r))
				do {
					auto next_arg = assignment_exp();
					call->add(next_arg);
				} while (match(token::comma));
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
		return exp;
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
// 		if (check(token::paren_l) && check1(token::type_name)) {	// cannot be matched, yet
// 			consume(token::paren_l, "Did check this already");
// 			auto type = type_name();
// 			auto closing = consume(token::paren_r, "Expect ')' after cast target type.");
// 			auto subexp = cast_exp();
// 			return make_node<cast>(closing, type, subexp);
// 		}
// 		return unary_exp();
		if (check(token::paren_l)) {
			int restart = current;   // error recovery point
			consume(token::paren_l, "Did check this already");
			auto type = type_name(); // this checks for ID, not TYPENAME, so will succeed
			if (check(token::paren_r)) {
				auto closing = consume(token::paren_r, "Expect ')' after cast target type.");
				auto subexp = cast_exp();
				return make_node<cast>(closing, type, subexp);
			}
			else try {
				current = restart;
				return unary_exp();
			}
			catch (parse_error e) {
				throw parse_error(e.at, std::string(e.what()) + "\nOr missing ')' after cast target type.");
			}
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
	
	
	/* 
	 * Statements.
	 *
	 */

	rule(declaration_specifiers) {
		// parse all possible specifiers into one node
		auto all = make_node<ast::declaration_specifiers>();
		while (true) {
			if (match(token::kw_void, token::kw_char, token::kw_int, token::kw_float, token::kw_double))
				all->add(make_node<ast::type_name>(previous()));
			else if (match(token::identifier))	// here we add any ID as a type name, see declaration_specifiers::add(type_name*)
				all->add(make_node<ast::type_name>(previous()));
			else if (match(token::kw_unsigned, token::kw_signed, token::kw_long, token::kw_short))
				all->add(make_node<ast::type_modifier>(previous()));
			else if (match(token::kw_const, token::kw_volatile, token::kw_auto, token::kw_static, token::kw_register, token::kw_extern))
				all->add(make_node<ast::type_qualifier>(previous()));
			else if (match(token::kw_struct, token::kw_union)) {
			}
			else if (match(token::kw_enum)) {
			}
			else break;
			cout << "consumed " << previous() << endl;
		}
		return all;
	};

	
	prrule(declarator, pointer_to<ast::declaration_specifiers> spec, declarator) {
		auto decl = make_node<ast::declarator>();
		while (match(token::star)) {
			bool c = false, v = false;
			if (match(token::kw_const))    c = true;
			if (match(token::kw_volatile)) v = true;
			decl->add_pointer(c, v);
		}
		if (match(token::identifier)) {
			decl->name = make_node<ast::identifier>(previous());
		}
		else if (match(token::paren_l)) {
			auto nested = declarator(nullptr);
			consume(token::paren_r, "Expect ')' after nested declarator.");
		}
		else { // we don't have a name but need one, maybe it was parsed as part of the decl-spec?
			if (spec && spec->last_id) {
				decl->name = make_node<ast::identifier>(spec->last_id->name);
				// XXX delete spec->last_id
				spec->last_id = nullptr;
				spec->specifiers.pop_back();
			}
			else
				throw parse_error(peek(), "Expect name for declarator.");
		}
		return decl;
	};

	rule(external_declaration) {
		// declaration and function-definition share this part
		auto spec = declaration_specifiers();
		auto decl = declarator(spec);
		if (match(token::brace_l)) {
			// we have a function definition
		}
		// we have a declaration
		pointer_to<ast::expression> init = nullptr;
		if (match(token::equals)) {
			init = assignment_exp(); // TODO other cases
		}
		if (!match(token::semicolon))
			throw parse_error(peek(), "Expect ';' at end of declaration");
		return make_node<ast::declaration>(spec, decl, init);
	};
	
	rule(translation_unit) {
		auto root = make_node<ast::translation_unit>();
		while (!at_end()) {
			root->add(external_declaration());
			cout << "next " << peek() << endl;
		}
		return root;
	};


	auto x = translation_unit();
	ast::printer p(std::cout);
	x->traverse_with(&p);
}

/*
	Excerpt of C grammar to parse declarations.
	How to figure if an identifier is part of the type or the declared name?
   
		external-declaration:
			function-definition
			declaration

	First branch

		declaration:
			declaration-specifiers init-declarator* ;
		
		declaration-specifiers:
			storage-class-specifier declaration-specifiers?
			type-specifier declaration-specifiers?
			type-qualifier declaration-specifiers?

		storage-class-specifier:
			auto | ... | typedef
		
		type-specifier:
			void | ... | unsigned | struct-or-union-specifier | enum-specifier | typedef-name

		typedef-name:
			identifier       // !!

		type-qualifier:
			const | volatile

		struct-or-union-spec:
			s-o-u identifier? { struct-declaration-list }
			s-o-u identifier

		s-o-u:
			struct | union

		enum-specifier:
			enum identifier? { enumerator-list }
			enum identifier

		init-declarator:
			declarator
			declarator = ...

		declarator:
			pointer? direct-declarator

		direct-declarator:
			identifier
			( identifier ... )
			identifier [ ... ]
			identifier ( ... )

	Second branch:

		function-definition:
			declaration-specifiers? declarator declaration-list compound-statement


	Declarations boil down to:

		declaration:
			s-o-u [...]
			enum [...]
			void | ... | unsigned | auto | ... | typedef | const | volatile | typedef-name[aka identifier]

	Suggest this approach:
	Eat up all decl-specs and hand in list-node to declarator-parser to take out the ID if necessary.
	

 */


// vim: filetype=cpp-parse
