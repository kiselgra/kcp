#include <stdio.h>

void seq_rec(int from, int to) {
	if (from > to)
		return;
	printf("%d\n", from);
	seq_rec(from+1, to);
}

void seq_while(int from, int to) {
	int i = from;
	while (i <= to) {
		printf("%d\n", i);
		i = i + 1;
	}
}

int main() {
// 	seq_rec(1, 10);
	seq_while(1, 10);
	return 0;
}
