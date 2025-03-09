int main(int argc, char **argv) {
	int i = 0;
	while (i < argc)
		printf("%s\n", argv[i]);

	i = 0;
	while (i < argc) {
		printf("%s\n", argv[i]);
	}

	i = 0;
	do 
		printf("%s\n", argv[i]);
	while (i < 0);

	i = 0;
	do {
		printf("%s\n", argv[i]);
	}
	while (i < 0);

	for (int i = 0; i < argc; ++i)
		printf("%s\n", argv[i]);
	
	for (int i = 0; i < argc; ++i) {
		printf("%s\n", argv[i]);
	}

	for (;;)
		break;
}
