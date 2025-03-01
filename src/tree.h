#pragma once

#include "token.h"

#include <ostream>
#include <string>
#include <vector>

namespace ast {
	using std::vector;
	
	template<typename T> using pointer_to = T*;
	template<typename T> T* unwrap(pointer_to<T> p) { return p; }
	template<typename T> bool is(pointer_to<T> node) {
		return dynamic_cast<T*>(unwrap(node)) != nullptr;
	}


	struct node {
// 		std::string 
		template<typename T> bool is() { return dynamic_cast<T*>(this) != nullptr; }
		virtual ~node() {}
	};

	struct expression;
	struct conditional;
	struct n_ary;
	struct sequence;
	struct arith;
	struct literal;
	struct number;

	struct visitor {
		#define forward(X) visit((X*)node)
		virtual void visit(expression  *node) {}
		virtual void visit(conditional *node) { forward(expression); }
		virtual void visit(n_ary       *node) { forward(expression); }
		virtual void visit(sequence    *node) { forward(n_ary); }
		virtual void visit(arith       *node) { forward(n_ary); }
		virtual void visit(literal     *node) { forward(expression); }
		virtual void visit(number      *node) { forward(literal); }
		#undef forward
	};


	struct expression : public node {
		virtual void traverse_with(visitor *) = 0;
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

	struct literal : public expression {
		::token token;
		literal(::token token) : token(token) {}
	};

	struct number : public literal {
		number(::token t) : literal(t) {}
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
// 		void visit(n_ary *node) override {
// 			int prev_ind = indent;
// 			if (std::reduce(
// 			out << ind() << 
// // 			out << ind() << "(" << node->op.text;
// // 			indent += 2 + node->op.text.length();
// // 			if (node->lhs->is<literal>())
// // 				out << " ";
// // 			node->lhs->traverse_with(this);
// // 			if (node->lhs->is<literal>() && node->rhs->is<literal>())
// // 				out << " ";
// // 			else if (node->rhs->is<literal>())
// // 				out << ind();
// // 			node->rhs->traverse_with(this);
// // 			out << ")";
// 			indent = prev_ind;
// 		}
		void visit(number *n) override;

	};
}
