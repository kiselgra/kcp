int foo(void) {
	typedef int a, b;
	a b; // b will fail as it is not an ID
}
