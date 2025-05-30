#include "token.h"
#include "parser.h"

#include <iostream>

using std::cout, std::endl, std::cerr;

int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "Args!" << endl;
		return -1;
	}
	try {
		auto tokens = lex_input(argv[1]);
// 		for (auto t : tokens) {
// 			cout << " - " << token::type_string(t.type) << ": " << t.text << " @" << t.line << "." << t.pos << endl;
// 			cout << " * " << t << endl;
// 		}
		parse(tokens);
	}
	catch (lexer_error e) {
		cerr << e.what() << endl;
		return -1;
	}
	catch (parse_error e) {
		cerr << e.what() << endl;
		return -1;
	}
	return 0;
}
