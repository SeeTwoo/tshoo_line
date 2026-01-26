#include <stdio.h>
#include <unistd.h>

int	main(void) {
	printf("hello, world\n");
	write(2, "hello, world\n", 13);
	return 0;
}
