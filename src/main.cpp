#include "token.h"

#include <iostream>

using std::cout, std::endl;

int main() {
	auto tokens = lex_input("src/main.cpp");
	for (auto t : tokens) {
		cout << " - " << token::type_name(t.type) << ": " << t.text << " @" << t.line << "." << t.pos << endl;
	}
	return 0;
}
