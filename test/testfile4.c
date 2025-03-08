int fak(int n) {
	if (n <= 1)
		return 1;
	return n * fak(n-1);
}

void random_break(void) {
	while (rand() % 100 != 0) if (a) break; else continue;
	goto togo;
}

void foo(int) {
}

void foo() {
}

void foo (int, char c, ...) {
}

int main(int argc, char **argv) {
	fak(100);
	random_break();
	printf("hallo!\"\"blub\n");
}
