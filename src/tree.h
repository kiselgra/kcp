#pragma once

#include "token.h"

#include <ostream>
#include <string>
#include <vector>

namespace ast {
	using std::vector;
	using std::tuple;
	
	template<typename T> using pointer_to = T*;
	template<typename T> T* unwrap(pointer_to<T> p) { return p; }
	template<typename T, typename P> bool is(pointer_to<P> node) {
		return dynamic_cast<T*>(unwrap(node)) != nullptr;
	}


	struct node;
	struct expression;
	struct conditional;
	struct n_ary;
	struct sequence;
	struct arith;
	struct logical;
	struct equality;
	struct relational;
	struct cast;
	struct unary;
	struct prefix;
	struct postfix;
	struct call;
	struct subscript;
	struct member_access;
	struct identifier;
	struct literal;
	struct number_lit;
	struct integral_lit;
	struct float_lit;
	struct character_lit;
	struct string_lit;
	
	struct translation_unit;
	struct type_specifier;
	struct type_name;
	struct type_modifier;
	struct type_qualifier;
	struct declaration_specifiers;
	struct declarator;
	struct declaration;
	struct struct_union;

	struct visitor {
		#define forward(X) visit((X*)node)
		virtual void visit(expression    *node) {}
		virtual void visit(conditional   *node) { forward(expression); }
		virtual void visit(n_ary         *node) { forward(expression); }
		virtual void visit(sequence      *node) { forward(n_ary); }
		virtual void visit(arith         *node) { forward(n_ary); }
		virtual void visit(logical       *node) { forward(n_ary); }
		virtual void visit(equality      *node) { forward(n_ary); }
		virtual void visit(relational    *node) { forward(n_ary); }
		virtual void visit(cast          *node) { forward(expression); }
		virtual void visit(unary         *node) { forward(expression); }
		virtual void visit(prefix        *node) { forward(unary); }
		virtual void visit(postfix       *node) { forward(unary); }
		virtual void visit(call          *node) { forward(expression); }
		virtual void visit(subscript     *node) { forward(expression); }
		virtual void visit(member_access *node) { forward(expression); }
		virtual void visit(identifier    *node) { forward(expression); }
		virtual void visit(literal       *node) { forward(expression); }
		virtual void visit(number_lit    *node) { forward(literal); }
		virtual void visit(integral_lit  *node) { forward(number_lit); }
		virtual void visit(float_lit     *node) { forward(number_lit); }
		virtual void visit(character_lit *node) { forward(literal); }
		virtual void visit(string_lit    *node) { forward(literal); }
		
		virtual void visit(translation_unit       *node) {}
		virtual void visit(type_specifier         *node) {}
		virtual void visit(type_name              *node) { forward(type_specifier); }
		virtual void visit(type_modifier          *node) { forward(type_specifier); }
		virtual void visit(type_qualifier         *node) { forward(type_specifier); }
		virtual void visit(declaration_specifiers *node) {}
		virtual void visit(declarator             *node) {}
		virtual void visit(declaration            *node) {}
		virtual void visit(struct_union           *node) { /*XXX*/ }
		#undef forward
	};

	struct node {
// 		std::string 
		template<typename T> bool is() { return dynamic_cast<T*>(this) != nullptr; }
		virtual ~node() {}
		
		virtual void traverse_with(visitor *) = 0;
	};


	struct expression : public node {
// 		virtual void traverse_with(visitor *) = 0; ??
	};

	struct conditional : public expression {
		token qmark, colon;
		pointer_to<expression> condition, consequent, alternative;
		conditional(pointer_to<expression> condition, token qmark, pointer_to<expression> consequent, token colon, pointer_to<expression> alternative)
		: qmark(qmark), colon(colon), condition(condition), consequent(consequent), alternative(alternative) {
		}
		virtual void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct n_ary : public expression {
		vector<token> infix_ops;
		vector<pointer_to<expression>> operands;
		n_ary(token op, pointer_to<expression> lhs, pointer_to<expression> rhs) {
			infix_ops.push_back(op);
			operands.push_back(lhs);
			operands.push_back(rhs);
		}
		void add(token next_operator, pointer_to<expression> next_operand) {
			infix_ops.push_back(next_operator);
			operands.push_back(next_operand);
		}
		void traverse_with(visitor *v) override { v->visit(this); }
	};
	
	struct sequence : public n_ary {
		sequence(token comma, pointer_to<expression> lhs, pointer_to<expression> rhs) : n_ary(comma, lhs, rhs) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};
		
	struct assign : public n_ary {
		assign(token kind, pointer_to<expression> lhs, pointer_to<expression> rhs) : n_ary(kind, lhs, rhs) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};
	
	struct arith : public n_ary {
		arith(token op, pointer_to<expression> lhs, pointer_to<expression> rhs) : n_ary(op, lhs, rhs) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct bitwise : public n_ary {
		bitwise(token op, pointer_to<expression> lhs, pointer_to<expression> rhs) : n_ary(op, lhs, rhs) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct logical : public n_ary {
		logical(token op, pointer_to<expression> lhs, pointer_to<expression> rhs) : n_ary(op, lhs, rhs) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct equality : public n_ary {
		equality(token op, pointer_to<expression> lhs, pointer_to<expression> rhs) : n_ary(op, lhs, rhs) {}
		void traverse_with(visitor *v) override { v->visit(this); }
};

	struct relational : public n_ary {
		relational(token op, pointer_to<expression> lhs, pointer_to<expression> rhs) : n_ary(op, lhs, rhs) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct cast : public expression {
		::token closing_paren;
		pointer_to<expression> type; // is this an expression?
		pointer_to<expression> expr;
		cast(::token closing_paren, pointer_to<expression> type, pointer_to<expression> expr) : closing_paren(closing_paren), type(type), expr(expr) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct unary : public expression {
		::token op;
		pointer_to<expression> sub;
		unary(::token op, pointer_to<expression> sub) : op(op), sub(sub) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct prefix : public unary {
		prefix(::token op, pointer_to<expression> sub) : unary(op, sub) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct postfix : public unary {
		postfix(::token op, pointer_to<expression> sub) : unary(op, sub) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct call : public expression {
		::token opening_paren;
		pointer_to<expression> callee;
		vector<pointer_to<expression>> arguments;
		call(::token opening_paren, pointer_to<expression> callee) : opening_paren(opening_paren), callee(callee) {}
		void add(pointer_to<expression> arg) {
			arguments.push_back(arg);
		}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct subscript : public expression {
		::token opening_bracket;
		pointer_to<expression> array, index;
		subscript(::token opening_bracket, pointer_to<expression> array, pointer_to<expression> subscript) : opening_bracket(opening_bracket), array(array), index(index) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct member_access : public expression {
		::token accessor;
		pointer_to<expression> outer, inner;
		member_access(::token accessor, pointer_to<expression> outer, pointer_to<expression> inner) : accessor(accessor), outer(outer), inner(inner) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct identifier : public expression {
		::token token;
		identifier(::token token) : token(token) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};
	
	struct literal : public expression {
		::token token;
		literal(::token token) : token(token) {}
	};

	struct number_lit : public literal {
		number_lit(::token t) : literal(t) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct integral_lit : public number_lit {
		integral_lit(::token t) : number_lit(t) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct float_lit : public number_lit {
		float_lit(::token t) : number_lit(t) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct character_lit : public literal {
		character_lit(::token t) : literal(t) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct string_lit : public literal {
		string_lit(::token t) : literal(t) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};



// 	struct statement; // move up

	struct translation_unit : public node {
		vector<pointer_to<node>> toplevel; // XXX 
		void add(pointer_to<node> stmt) {  // XXX use statement node type
			toplevel.push_back(stmt);
		}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

// 	struct statement : public node {
// 		void traverse_with(visitor *v) override { v->visit(this); }
// 	};
// 
// 	struct function_definition : public statement {
// 		void traverse_with(visitor *v) override { v->visit(this); }
// 	};

	struct type_specifier : public node {
		::token name;
		type_specifier(::token name) : name(name) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	// void, int, float, double
	struct type_name : public type_specifier {
		type_name(::token name) : type_specifier(name) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	// unsigned, signed, short, long
	struct type_modifier : public type_specifier {
		type_modifier(::token name) : type_specifier(name) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	// const volatile
	struct type_qualifier : public type_specifier {
		type_qualifier(::token name) : type_specifier(name) {}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct struct_union : public node { // XXX what base should this use?
		token kind;
		pointer_to<identifier> struct_name = nullptr, union_name = nullptr;
		vector<pointer_to<declaration>> declarations;
		struct_union(token kind, pointer_to<identifier> name) : kind(kind) {
			if (kind == token::kw_struct)
				struct_name = name;
			else
				union_name = name;
		}
		pointer_to<identifier> name() {
			return struct_name ? struct_name : union_name;
		}
		void add(pointer_to<declaration> decl) {
			declarations.push_back(decl);
		}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct declaration_specifiers : public node {
		vector<pointer_to<type_specifier>> specifiers;
		declaration_specifiers() = default;
		pointer_to<node> type = nullptr; // XXX do we have a more concrete type for "type"?
		void add(pointer_to<type_specifier> spec) {
			specifiers.push_back(spec);
		}
		void add(pointer_to<type_name> spec) {
			specifiers.push_back(spec);
		}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct declarator : public node {
		struct pointer_qualifier {
			bool c, v;
		};
		vector<pointer_qualifier> pointer;
		vector<pointer_to<expression>> array;  // nullptr-entries correspond do unsized dimensions
		vector<pointer_to<declaration>> fn_params; // if single entry is nullptr then this has no specified arguments (ie arbitrary)
		pointer_to<identifier> name;
		void add_pointer(bool c, bool v) {
			pointer.push_back({c, v});
		}
		void add_array(pointer_to<expression> array_size) {
			array.push_back(array_size);
		}
		void add_parameter(pointer_to<declaration> param) {
			fn_params.push_back(param);
		}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	struct declaration : public node {
		pointer_to<declaration_specifiers> specifiers;
		vector<tuple<pointer_to<declarator>,
		             pointer_to<expression>,
		             pointer_to<expression>>> init_declarators;
		declaration(pointer_to<declaration_specifiers> specifiers) : specifiers(specifiers) {
		}
		declaration(pointer_to<declaration_specifiers> specifiers,
					pointer_to<declarator> declarator,
					pointer_to<expression> initializer = nullptr)
		: specifiers(specifiers) {
			add_init_decl(declarator, initializer);
		}
		void add_init_decl(pointer_to<declarator> declarator, pointer_to<expression> initializer = nullptr) {
			init_declarators.push_back({declarator, initializer, nullptr});
		}
		void add_width_decl(pointer_to<declarator> declarator, pointer_to<expression> fieldwidth) {
			init_declarators.push_back({declarator, nullptr, fieldwidth});
		}
		void traverse_with(visitor *v) override { v->visit(this); }
	};

	template<typename T, typename... Args> pointer_to<T> make_node(Args... args) {
		return new T(std::forward<Args>(args)...);
	}



	struct printer : public visitor {
		std::ostream &out;
		int indent_size = 0;
		printer(std::ostream &out) : out(out) {}
		
		std::string ind() { return "\n"+std::string(indent_size, ' '); }
		struct indent_block {
			printer *p;
			indent_block(printer *p) : p(p) { 
				p->indent_size += 2;
			}
			~indent_block() {
				p->indent_size -= 2;
			}
		};

		void visit(conditional *node) override;
		void visit(n_ary *node) override;
		void visit(cast *node) override;
		void visit(unary *node) override;
		void visit(call *node) override;
		void visit(subscript *node) override;
		void visit(member_access *node) override;
		void visit(identifier *n) override;
		void visit(literal *n) override;
		
		void visit(translation_unit *n) override;
		void visit(declaration_specifiers *n) override;
		void visit(declarator *n) override;
		void visit(declaration *n) override;
		void visit(struct_union *n) override;

	};

	void print(pointer_to<node> ast);
}
