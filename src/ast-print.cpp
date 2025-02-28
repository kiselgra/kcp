#include "tree.h"

#include <numeric>
#include <typeinfo>
#include <string>

using std::string;

namespace ast {

	void printer::visit(n_ary *node) {
		int prev_ind = indent;
		if (std::transform_reduce(node->operands.begin(), node->operands.end(), true, std::logical_and{},
						[](pointer_to<expression> e){ return e->is<literal>(); })) {
			// all operands are literals
			out << ind() << "(";
			node->operands.front()->traverse_with(this);;
			for (int i = 0; i < node->infix_ops.size(); ++i) {
				out << " " << node->infix_ops[i].text << " ";
				node->operands[i+1]->traverse_with(this);
			}
			out << ")";
		}
		else {
			string name = typeid(*node).name();
			out << ind() << "(" << name;
			indent += 2;
			out << ind();
			node->operands.front()->traverse_with(this);
			for (int i = 0; i < node->infix_ops.size(); ++i) {
				out << ind() << node->infix_ops[i].text << " ";
				node->operands[i+1]->traverse_with(this);
			}
		}
		indent = prev_ind;
	}
	void printer::visit(number *n) {
		out << n->token.text;
	}

}
