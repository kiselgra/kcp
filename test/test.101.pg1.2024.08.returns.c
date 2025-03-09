#include <stdio.h>
#include <stdbool.h>

bool verbose = true;

int indent = 0;
void print_indent(int n) {
	if (n > 0) {
		printf(" ");
		print_indent(n-1);
	}
}
// λ
int fak(int n) {
	indent += 1;
	if (verbose) {
		print_indent(indent);
		printf("Aufruf fak(%d)\n", n);
	}
	if (n < 2) {        // **
		if (verbose) {
			print_indent(indent);
			printf("Rekursionsende bei n=%d\n", n);
		}
		return 1;       // **
	}
	int res = n * fak(n-1); // **
	if (verbose) {
		print_indent(indent);
		printf("Aufruf von fak(%d) ist fertig\n", n);
	}
	indent = indent-1;
	return res;   // **
}

int main() {
	printf("--> 10! = %d\n", fak(10));
	return 0;
}
