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
#define fprrule(X,P,Y) X = [&](P) -> pointer_to<ast::Y>

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
	helper(rewind1) {
		current--;
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
	std::function<pointer_to<ast::declaration>()> parameter_declaration;
	std::function<pointer_to<ast::declaration_specifiers>()> declaration_specifiers;
	std::function<pointer_to<ast::declarator>(bool)> declarator;

	rule(struct_declaration) {
		// a declaration inside of a struct
		auto spec = declaration_specifiers();
		auto declaration = make_node<ast::declaration>(spec);
		while (!match(token::semicolon)) {
			auto decl = declarator(false);
			pointer_to<ast::expression> fieldwidth = nullptr;
			if (match(token::colon))
				fieldwidth = conditional_exp();
			declaration->add_width_decl(decl, fieldwidth);
			if (!check(token::semicolon))
				consume(token::comma, "Expect ';' or ',' after struct declarator.");
		}
		return declaration;
	};

	prule(struct_or_union, token keyword) {
		// we have parsed the keyword already
		pointer_to<ast::identifier> name = nullptr;
		if (match(token::identifier))
			name = make_node<ast::identifier>(previous());
		auto structure = make_node<struct_union>(keyword, name);
		// if not named, declaration-list is not optional
		if (!name)
			consume(token::brace_l, "Expected '{' after anonymous struct or union.");
		else if (!match(token::brace_l)) // consumes the brace if present
			return structure;
		// parse declaration list
		while (!match(token::brace_r)) {
			auto decl = struct_declaration();
			structure->add(decl);
		}
		return structure;
	};

	frule(declaration_specifiers) {
		// parse all possible specifiers into one node
		auto all = make_node<ast::declaration_specifiers>();
		bool last_id = false;
		bool int_mod = false;
		vector<token> identifiers;
		while (true) {
			if (match(token::kw_void, token::kw_char, token::kw_int, token::kw_float, token::kw_double)) {
				last_id = false;
				all->type = make_node<ast::type_name>(previous());
			}
			else if (match(token::identifier))	{
				last_id = true;
				identifiers.push_back(previous());
			}
			else if (match(token::kw_unsigned, token::kw_signed, token::kw_long, token::kw_short)) {
				last_id = false;
				int_mod = true;
				all->add(make_node<ast::type_modifier>(previous()));
			}
			else if (match(token::kw_const, token::kw_volatile, token::kw_auto, token::kw_static, token::kw_register, token::kw_extern)) {
				last_id = false;
				all->add(make_node<ast::type_qualifier>(previous()));
			}
			else if (match(token::kw_struct, token::kw_union)) {
				last_id = false;
				all->type = struct_or_union(previous());
			}
			else if (match(token::kw_enum)) {
				last_id = false;
				// TODO
			}
			else break;
		}

		// we might have parsed one indentifier too many, if so, push it back.
		// to make this a little more safe, we first figure out what the typename was
			
		if (!all->type) { // if type is set already, then we have a struct,union,enum or one void,char,int,float,double
			if (int_mod)  // if there is unsigned, etc -> implicity type is int
				all->type = make_node<ast::type_name>(token(token::kw_int, "int", -1, -1));
			else if (identifiers.size() > 0) { // if there are unmatched identifiers, the first one is the type. We follow c99 by disallowing implicit int
				all->type = make_node<ast::type_name>(identifiers.front());
				identifiers.erase(identifiers.begin());
			}
		}
		// XXX this is a hack and might be problematic at some point...
		if (last_id && identifiers.size() > 0) {
			identifiers.pop_back();
			rewind1();
			assert(peek() == token::identifier);
		}
		if (identifiers.size() > 0) // XXX is this always the right thing to do?
			throw parse_error(identifiers.front(), "Superflous identifiers in declaration specifiers.");

		return all;
	};

	fprrule(declarator, bool allow_unnamed, declarator) {
		auto decl = make_node<ast::declarator>();
		// pointers
		while (match(token::star)) {
			bool c = false, v = false;
			if (match(token::kw_const))    c = true;
			if (match(token::kw_volatile)) v = true;
			decl->add_pointer(c, v);
		}
		// name / nesting
		if (match(token::identifier)) {
			decl->name = make_node<ast::identifier>(previous());
		}
		else if (match(token::paren_l)) {
			auto nested = declarator(true); // XXX how to hook this in?
			consume(token::paren_r, "Expect ')' after nested declarator.");
		}
		else if (!allow_unnamed)
			throw parse_error(peek(), "Expect name or nested declaration for declarator.");
		// arrays and functions
		if (match(token::bracket_l)) {
			while (match(token::bracket_l)) {
				pointer_to<ast::expression> const_expr = nullptr;
				if (!check(token::bracket_r))
					const_expr = conditional_exp();
				decl->add_array(const_expr);
				consume(token::bracket_r, "Expect ']' after array dimension.");
			}
			if (match(token::paren_l))
				throw parse_error(previous(), "Array of functions not allowed.");
		}
		else if (match(token::paren_l)) {
			if (!check(token::paren_r))
				do decl->add_parameter(parameter_declaration());
				while (match(token::comma));
			else
				decl->add_parameter(nullptr); // meaning: function, but no specified params
			consume(token::paren_r, "Expect ')' after function declaration");
		}
		return decl;
	};
	
	// TODO test a function w/o params, test one with, add visitor for this
	frule(parameter_declaration) {
		auto spec = declaration_specifiers();
		auto decl = declarator(true);
		// this can also be w/o declarator or with "abstract-declarator" (TODO)
		return make_node<ast::declaration>(spec, decl);
	};

	rule(external_declaration) {
		// declaration and function-definition share this part
		auto spec = declaration_specifiers();
		auto declaration = make_node<ast::declaration>(spec); // XXX do we "steal" the ID for the second declarator?
		while (!match(token::semicolon)) {
			auto decl = declarator(false);
			if (match(token::brace_l)) {
				// we have a function definition
				// TODO
			}
			// we have a declaration
			pointer_to<ast::expression> init = nullptr;
			if (match(token::equals)) {
				init = assignment_exp(); // TODO other cases
			}
			declaration->add_init_decl(decl, init);
			if (!check(token::semicolon))
				consume(token::comma, "Expect ';' or ',' after declarator.");
		}
		return declaration;
	};
	
	rule(translation_unit) {
		auto root = make_node<ast::translation_unit>();
		while (!at_end()) {
			cout << "next " << peek() << endl;
			root->add(external_declaration());
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
