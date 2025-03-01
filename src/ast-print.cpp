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
	void printer::visit(number *n) {
		out << ind() << n->token.text;
	}

}
