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

	void printer::visit(identifier *node) {
		out << ind() << node->token.text;
	}


	void printer::visit(translation_unit *node) {
		for (auto x : node->toplevel)
			x->traverse_with(this);
	}
	
	void printer::visit(declaration_specifiers *node) {
		out << ind() << "(decl-spec";
		for (auto x : node->specifiers)
			out << " " << x->name.text;
		if (node->last_id)
			out << " | last-id";
		out << ")";
	}
	
	void printer::visit(declarator *node) {
		header("declarator");
		for (auto p : node->pointer) {
			out << " *";
			if (p.c) out << " const";
			if (p.v) out << " volatile";
		}
		node->name->traverse_with(this);
		out << ")";
	}

	void printer::visit(declaration *node) {
		header("declaration");
		if (node->specifiers)  node->specifiers->traverse_with(this);
		if (node->declarator)  node->declarator->traverse_with(this);
		if (node->initializer) node->initializer->traverse_with(this);
		out << ")";	
	}

	void printer::visit(literal *node) {
		out << ind() << node->token.text;
	}

}
