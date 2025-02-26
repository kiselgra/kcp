#include "token.h"

#include <iostream>

using std::cout, std::endl;

int main() {
	auto tokens = lex_input("src/main.cpp");
	for (auto t : tokens) {
		cout << " - " << token::type_name(t.type) << ": " << t.text << endl;
	}
	return 0;
}
