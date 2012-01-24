#include <stdio.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
	int c;
	char str[2];

	str[1] = 0;
	while ((c = getchar()) != EOF) {
		if (c == '\n')
			continue;
		str[0] = c;
		putchar(atoi(str));
	}

	return 0;
}
