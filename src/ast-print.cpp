#include "tree.h"

#include <numeric>
#include <typeinfo>
#include <string>

using std::string;

namespace ast {

#define indent indent_block indent_for_this_node(this)
#define header(X) out << ind() << "(" << X; indent

	void printer::visit(conditional *node) {
		header("conditional");
		node->condition->traverse_with(this);
		node->consequent->traverse_with(this);
		node->alternative->traverse_with(this);
		out << ")";
	}
	void printer::visit(n_ary *node) {
		string name = typeid(*node).name();
		header(name);
		if (std::transform_reduce(node->operands.begin(), node->operands.end(), true, std::logical_and{},
						[](pointer_to<expression> e){ return e->is<literal>(); })) {
			// all operands are literals
			node->operands.front()->traverse_with(this);;
			for (int i = 0; i < node->infix_ops.size(); ++i) {
				out << " " << node->infix_ops[i].text << " ";
				node->operands[i+1]->traverse_with(this);
			}
		}
		else {
			node->operands.front()->traverse_with(this);
			for (int i = 0; i < node->infix_ops.size(); ++i) {
				out << ind() << node->infix_ops[i].text << " ";
				node->operands[i+1]->traverse_with(this);
			}
		}
		out << ")";
	}
	
	void printer::visit(cast *node) {
		header("cast");
		node->type->traverse_with(this);
		node->expr->traverse_with(this);
		out << ")";
	}

	void printer::visit(unary *node) {
		header("unary");
		out << " " << node->op.text;
		node->sub->traverse_with(this);
		out << ")";
	}

	void printer::visit(call *node) {
		header("call");
		node->callee->traverse_with(this);
		for (auto arg : node->arguments)
			arg->traverse_with(this);
		out << ")";
	}

	void printer::visit(subscript *node) {
		header("subscript");
		node->array->traverse_with(this);
		node->index->traverse_with(this);
		out << ")";
	}

	void printer::visit(member_access *node) {
		header("member-access");
		node->outer->traverse_with(this);
		node->inner->traverse_with(this);
		out << ")";
	}

	void printer::visit(identifier *n) {
		out << ind() << n->token.text;
	}

	void printer::visit(literal *n) {
		out << ind() << n->token.text;
	}

}
