#include "parser.h"
#include "tree.h"

#include <iostream>
#include <functional>
#include <set>
#include <cassert>

using namespace std;
using namespace ast;

#define helper(X,...) auto X = [&](__VA_ARGS__)
#define rule(X) auto X = [&]()
#define prule(X,P) auto X = [&](P)
#define frule(X) X = [&]()
#define pfrule(X,P) X = [&](P)
#define rrule(X,Y) std::function<pointer_to<ast::Y>()> X = [&]() -> pointer_to<ast::Y>
#define prrule(X,P,Y) std::function<pointer_to<ast::Y>(P)> X = [&](P) -> pointer_to<ast::Y>
#define fprrule(X,P,Y) X = [&](P) -> pointer_to<ast::Y>

static constexpr bool verbose = true;
#define log if (verbose) cout

struct scope {
	token scope_head;
	std::set<std::string> typenames;
	scope(token head) : scope_head(head) {}
	void define(token t) {
		typenames.insert(t.text);
	}
	bool defined(const std::string &name) {
		return typenames.contains(name);
	}
};

void parse(const vector<token> &tokens) {

	int current = 0;
	
	// symbol table for lexer feedback
	vector<scope> scopes;
	helper(push_scope, token head) {
		scopes.emplace_back(head);
	};
	helper(pop_scope) {
		scopes.pop_back();
	};
	helper(register_type, token id) {
		scopes.back().define(id);
	};
	helper(is_type, token t) {
		if (t == token::identifier)
			for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
				if (it->defined(t.text))
					return true;
		return false;
	};
	helper(as_type, token t) {
		return token(token::type_name, t.text, t.line, t.pos);
	};
	helper(fix_token, token t) {
		if (is_type(t))
			return as_type(t);
		return t;
	};
	push_scope(token(token::eof, "global scope", -1, -1));
	
	// token access
	helper(at_end) {
		return tokens[current] == token::eof;
	};
	helper(previous) {
		assert(current > 0);
		return fix_token(tokens[current-1]);
	};
	helper(advance) {
		if (!at_end()) current++;
		return previous();
	};
	helper(peek) {
		return fix_token(tokens[current]);
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
	helper(peek1) {
		if (current < tokens.size())
			return fix_token(tokens[current+1]);
		return fix_token(tokens[current]); // will be eof
	};
	helper(check1, enum token::type t) {
		return peek1() == t;
	};
	helper(log_tokens, int N) {
		log << "tokenstream excerpt: ";
		for (int i = 0; i < N; ++i)
			if (current+i < tokens.size())
				log << tokens[i];
		log << endl;
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
		return parse_nary.template operator()<ast::arith>(cast_exp, token::star, token::slash, token::percent);
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
														   token::left_left_equals, token::right_right_equals, token::percent_equals, token::amp_equals, token::hat_equals, token::pipe_equals);
	};
	frule(expression) {
		return parse_nary.template operator()<sequence>(assignment_exp, token::comma);
	};
	
	rule(expression_statement) {
		auto exp = expression();
		consume(token::semicolon, "Expect a ';' after expression.");
		return make_node<ast::expression_stmt>(exp);
	};
	
	/* 
	 * Statements.
	 *
	 */
	std::function<pointer_to<ast::declaration>()> parameter_declaration;
	std::function<pointer_to<ast::declaration_specifiers>()> declaration_specifiers;
	std::function<pointer_to<ast::declarator>(bool)> declarator;
	std::function<pointer_to<ast::declaration>(bool)> external_declaration;;
	
	std::function<pointer_to<ast::statement>()> statement;	//XXX might be statement
	std::function<pointer_to<ast::block>()> compound_statement;

	helper(next_is_expression) {
		token t = peek();
		switch (t.type) {
		case token::identifier:
		case token::integral: case token::floating: case token::character: case token::string: 
		case token::star: case token::paren_l: case token::minus: case token::minus_minus:
			return true;
		default:
			return false;
		}
	};

	rule(if_statement) {
		consume(token::paren_l, "Expect '(' after 'if'");
		auto condition = expression();
		consume(token::paren_r, "Expect ')' after 'if' condition");
		auto consequent = statement();
		pointer_to<ast::statement> alternate = nullptr;
		if (match(token::kw_else))
			alternate = statement();
		return make_node<if_stmt>(condition, consequent, alternate);
	};
	rule(switch_statement) {
		consume(token::paren_l, "Expect '(' after 'switch'");
		auto condition = expression();
		consume(token::paren_r, "Expect ')' after 'switch' expression");
		auto body = statement();
		return make_node<switch_stmt>(condition, body);
	};
	prule(return_statement, token t) {
		if (match(token::semicolon))
			return make_node<return_stmt>(t);
		auto expr = expression();
		consume(token::semicolon, "Expect ';' after return expression.");
		return make_node<return_stmt>(t, expr);
	};
	prule(break_statement, token t) {
		consume(token::semicolon, "Expect ';' after 'break'.");
		return make_node<break_stmt>(t);
	};
	prule(continue_statement, token t) {
		consume(token::semicolon, "Expect ';' after 'continue'.");
		return make_node<continue_stmt>(t);
	};
	prule(goto_statement, token t) {
		auto id = identifier();
		consume(token::semicolon, "Expect ';' after goto label.");
		return make_node<goto_stmt>(t, id);
	};
	prule(case_statement, token t) {
		auto id = conditional_exp();
		consume(token::colon, "Expect ':' after case label.");
		return make_node<label_stmt>(t, id);
	};
	prule(default_statement, token t) {
		consume(token::colon, "Expect ':' after default label.");
		return make_node<label_stmt>(t);
	};
	rule(while_statement) {
		consume(token::paren_l, "Expect '(' after while.");
		auto test = expression();
		consume(token::paren_r, "Expect ')' after while condition.");
		auto stmt = statement();
		return make_node<while_loop>(test, stmt);
	};
	rule(dowhile_statement) {
		auto stmt = statement();
		consume(token::kw_while, "Expect 'while' after 'do ...'.");
		consume(token::paren_l, "Expect '(' after 'while'");
		auto test = expression();
		consume(token::paren_r, "Expect ')' after do-while condition.");
		consume(token::semicolon, "Expect ';' after do-while.");
		return make_node<dowhile_loop>(test, stmt);
	};
	rule(for_statement) {
		consume(token::paren_l, "Expect '(' after for.");
		pointer_to<ast::statement> init = nullptr;
		if (!match(token::semicolon))
			if (next_is_expression())
				init = expression_statement();
			else
				init = external_declaration(false);
		pointer_to<ast::expression> expr = nullptr;
		if (match(token::semicolon)) {
			expr = make_node<integral_lit>(token(token::integral, "1", -1, -1));
		}
		else {
			expr = expression();
			consume(token::semicolon, "Expect ';' after for condition.");
		}
		pointer_to<ast::expression> step = nullptr;
		if (!check(token::paren_r))
			step = expression();
		consume(token::paren_r, "Expect ')' after for.");
		auto body = statement();
		return make_node<for_loop>(init, expr, step, body);
	};

	// the loop body should be in statement()
	frule(statement) -> pointer_to<ast::statement> {
		if (verbose) {
			log << "at statement(): next_is_exp=" << next_is_expression() << endl;
			log_tokens(6);
		}
		// first one is a special case
		if (check(token::identifier) && check1(token::colon)) {
			auto id = identifier();
			advance();
			return make_node<label_stmt>(id);
		}
		// then check the keyword-driven cases in order
		if (next_is_expression())      return expression_statement();
		if (match(token::kw_if))       return if_statement();
		if (match(token::kw_switch))   return switch_statement();
		if (match(token::kw_return))   return return_statement(previous());
		if (match(token::kw_break))    return break_statement(previous());
		if (match(token::kw_continue)) return continue_statement(previous());
		if (match(token::kw_goto))     return goto_statement(previous());
		if (match(token::kw_case))     return case_statement(previous());
		if (match(token::kw_default))  return default_statement(previous());
		if (match(token::kw_while))    return while_statement();
		if (match(token::kw_do))       return dowhile_statement();
		if (match(token::kw_for))      return for_statement();
		if (match(token::brace_l))     return compound_statement();
		if (match(token::semicolon))   return make_node<block>();
		return external_declaration(false);
	};

	frule(compound_statement) {
		// the opening brace is consumed already
		push_scope(previous());
		vector<pointer_to<ast::statement>> statements;
		while (!match(token::brace_r)) {
			statements.push_back(statement());
		}
		pop_scope();
		return make_node<ast::block>(statements);
	};

	rule(struct_declaration) {
		// a declaration inside of a struct
		auto spec = declaration_specifiers();
		auto declaration = make_node<ast::var_declarations>(spec);
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
		push_scope(previous());
		while (!match(token::brace_r)) {
			auto decl = struct_declaration();
			structure->add(decl);
		}
		pop_scope();
		return structure;
	};
	rule(enum_specifier) {
		// we have parsed the enum keyword already
		pointer_to<ast::identifier> name = nullptr;
		if (match(token::identifier))
			name = make_node<ast::identifier>(previous());
		auto enumeration = make_node<ast::enumeration>(name);
		// if not named, enumerator-list is not optional
		if (!name)
			consume(token::brace_l, "Expected '{' after anonymous struct or union.");
		else if (!match(token::brace_l)) // consumes the brace if present
			return enumeration;
		while (true) {
			auto name = identifier();
			pointer_to<ast::expression> value = nullptr;
			if (match(token::equals))
				value = conditional_exp();
			enumeration->add(name, value);
			if (peek() == token::brace_r)
				break;
			else if (peek() == token::comma && peek1() == token::brace_r) {
				advance();
				break;
			}
			consume(token::comma, "Expect ',' or '}' after enumeration item.");
		}
		consume(token::brace_r, "Expect ',' or '}' after enumeration item.");
		return enumeration;
	};

	frule(declaration_specifiers) {
		// parse all possible specifiers into one node
		auto all = make_node<ast::declaration_specifiers>();
		bool int_mod = false;
		helper(type_duplicate_check) {
			if (all->type)
				throw parse_error(previous(), "Duplicate type in declaration.");
		};
		while (true) {
			if (match(token::kw_void, token::kw_char, token::kw_int, token::kw_float, token::kw_double)) {
				type_duplicate_check();
				all->type = make_node<ast::type_name>(previous());
			}
			else if (match(token::type_name))	{
				type_duplicate_check();
				all->type = make_node<ast::type_name>(previous());
			}
			else if (match(token::kw_unsigned, token::kw_signed, token::kw_long, token::kw_short)) {
				int_mod = true;
				all->add(make_node<ast::type_modifier>(previous()));
			}
			else if (match(token::kw_const, token::kw_volatile, token::kw_auto, token::kw_static, token::kw_register, token::kw_extern)) {
				all->add(make_node<ast::type_qualifier>(previous()));
				if (previous() == token::kw_register)
					int_mod = true;
			}
			else if (match(token::kw_struct, token::kw_union)) {
				type_duplicate_check();
				all->type = struct_or_union(previous());
			}
			else if (match(token::kw_enum)) {
				type_duplicate_check();
				all->type = enum_specifier();
			}
			else break;
		}

		if (!all->type)
			if (int_mod)  // if there is unsigned, etc -> implicity type is int
				all->type = make_node<ast::type_name>(token(token::kw_int, "int", -1, -1));
			else
				throw parse_error(peek(), "Expect type name for declaration.");
		
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
				do 
					if (match(token::ellipsis)) {
						decl->ellipsis = true;
						if (!check(token::paren_r))
							throw parse_error(peek(), "Expect ')' after '...'");
						break;
					}
					else
						decl->add_parameter(parameter_declaration());
				while (match(token::comma));
			else
				decl->add_parameter(nullptr); // meaning: function, but no specified params
			consume(token::paren_r, "Expect ')' after function declaration");
		}
		return decl;
	};
	
	frule(parameter_declaration) {
		auto spec = declaration_specifiers();
		auto decl = declarator(true);
		// this can also be w/o declarator or with "abstract-declarator" (TODO)
		return make_node<ast::var_declarations>(spec, decl);
	};

	pfrule(external_declaration, bool allow_function) -> pointer_to<ast::declaration> {
		// declaration and function-definition share this part
		auto spec = declaration_specifiers();
		auto declaration = make_node<ast::var_declarations>(spec);
		while (!match(token::semicolon)) {
			auto decl = declarator(false);
			if (match(token::brace_l) && allow_function) {
				// we have a function definition
				free_node(declaration);
				auto block = compound_statement();
				auto fdef = make_node<ast::function_definition>(spec, decl, block);
				return fdef;
			}
			// we have a declaration
			pointer_to<ast::expression> init = nullptr;
			if (match(token::equals)) {
				init = assignment_exp(); // TODO other cases, see "initializer"
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
			root->add(external_declaration(true));
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
