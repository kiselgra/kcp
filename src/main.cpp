#include "token.h"
#include "parser.h"

#include <iostream>

using std::cout, std::endl;

int main() {
	auto tokens = lex_input("testfile");
	for (auto t : tokens) {
		cout << " - " << token::type_name(t.type) << ": " << t.text << " @" << t.line << "." << t.pos << endl;
	}
	parse(tokens);
	return 0;
}
