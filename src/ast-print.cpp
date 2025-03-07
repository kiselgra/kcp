#include "tree.h"

#include <numeric>
#include <typeinfo>
#include <string>
#include <iostream>
#include <tuple>

using std::string;

std::string demangle(const std::string &name); // see below, copied from S/O

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
		name = demangle(name);
		if (name.find_last_of(":") != string::npos)
			name = name.substr(name.find_last_of(":")+1);
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

	void printer::visit(literal *node) {
		out << ind() << node->token.text;
	}
		
	void printer::visit(string_lit *node) {
		string escaped;
		for (char c : node->token.text)
			switch (c) {
			case '\n': escaped += "\\n";  break;
			case '\t': escaped += "\\t";  break;
			case '\r': escaped += "\\r";  break;
			case '"':  escaped += "\\\""; break;
			default:   escaped += c;
			}
		out << ind() << '"' << escaped << '"';
	}
	
	void printer::visit(translation_unit *node) {
		for (auto x : node->toplevel)
			x->traverse_with(this);
		out << "\n";
	}
	
	void printer::visit(declaration_specifiers *node) {
		out << ind() << "(decl-spec";
		for (auto x : node->specifiers)
			out << " " << x->name.text;
		if (node->type)
			node->type->traverse_with(this);
		out << ")";
	}
	
	void printer::visit(declarator *node) {
		header("declarator");
		for (auto p : node->pointer) {
			out << " *";
			if (p.c) out << " const";
			if (p.v) out << " volatile";
		}
		if (node->name)
			node->name->traverse_with(this);
		if (node->array.size()) {
			header("array");
			for (auto x : node->array)
				if (x)
					x->traverse_with(this);
				else
					out << ind() << "null";
			out << ")";
		}
		if (node->fn_params.size()) {
			header("fn-parameters");
			for (auto x : node->fn_params)
				if (x)
					x->traverse_with(this);
				else
					out << ind() << "null";
			out << ")";
		}
		out << ")";
	}

	void printer::visit(var_declarations *node) {
		header("declarations");
		if (node->specifiers)  node->specifiers->traverse_with(this);
		for (auto [decl,init,width] : node->init_declarators) {
			decl->traverse_with(this);
			if (init)  init->traverse_with(this);
			if (width) width->traverse_with(this);
		}
		out << ")";	
	}
		
	void printer::visit(function_definition *node) {
		header("function-definition");
		if (node->specifiers)
			node->specifiers->traverse_with(this);
		node->declarator->traverse_with(this);
		node->block->traverse_with(this);
		out << ")";	
	}
	
	void printer::visit(struct_union *node) {
		header(node->kind.text);
		if (node->name()) out << " " << node->name()->token.text;
		for (auto x : node->declarations)
			x->traverse_with(this);
	}

	void printer::visit(enumeration *node) {
		header("enum");
		if (node->name) out << " " << node->name->token.text;
		for (auto [n,v] : node->enumerators) {
			out << ind() << n->token.text;
			if (v) {
				out << " = ";
				v->traverse_with(this);
			}
		}
		out << ")";
	}

	void printer::visit(expression_stmt *node) {
		node->expression->traverse_with(this);
	}

	void printer::visit(if_stmt *node) {
		header("if");
		node->condition->traverse_with(this);
		node->consequent->traverse_with(this);
		if (node->alternate)
			node->alternate->traverse_with(this);
		out << ")";
	}

	void printer::visit(switch_stmt *node) {
		header("switch");
		node->expression->traverse_with(this);
		if (node->body)
			node->body->traverse_with(this);
		out << ")";
	}

	void printer::visit(jump_stmt *node) {
		header(node->kind.text);
		if (node->expression)
			node->expression->traverse_with(this);
		out << ")";
	}

	void printer::visit(label_stmt *node) {
		header("label");
		if (node->keyword) out << " " << node->keyword->text;
		if (node->label)
			node->label->traverse_with(this);
		out << ")";
	}

	void printer::visit(loop_stmt *node) {
		header("loop");
		node->condition->traverse_with(this);
		node->body->traverse_with(this);
		out << ")";
	}

	void printer::visit(for_loop *node) {
		header("for");
		if (node->init) node->init->traverse_with(this);
		if (node->condition) node->condition->traverse_with(this);
		if (node->step) node->step->traverse_with(this);
		node->body->traverse_with(this);
		out << ")";
	}

	void printer::visit(block *node) {
		header("block");
		for (auto x : node->statements) {
			x->traverse_with(this);
		}
		out << ")";
	}


	
	
	void print(pointer_to<node> ast) {
		printer p(std::cout);
		ast->traverse_with(&p);
	}

}


// https://stackoverflow.com/a/4541470
#include <cstdlib>
#include <memory>
#include <cxxabi.h>

std::string demangle(const std::string &name) {
	int status = -4; // some arbitrary value to eliminate the compiler warning
	// enable c++11 by passing the flag -std=c++11 to g++
	std::unique_ptr<char, void(*)(void*)> res {
		abi::__cxa_demangle(name.c_str(), NULL, NULL, &status),
			std::free
	};
	return (status==0) ? res.get() : name.c_str() ;
}

