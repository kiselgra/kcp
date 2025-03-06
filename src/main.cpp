#include "token.h"
#include "parser.h"

#include <iostream>

using std::cout, std::endl, std::cerr;

int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "Args!" << endl;
		return -1;
	}
	auto tokens = lex_input(argv[1]);
	for (auto t : tokens) {
		cout << " - " << token::type_string(t.type) << ": " << t.text << " @" << t.line << "." << t.pos << endl;
		cout << " * " << t << endl;
	}
	parse(tokens);
	return 0;
}
