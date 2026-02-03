#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int	main(int ac, char **av) {
	if (ac != 3)
		return 0;

	int	term_width = 10;
	int	x = atoi(av[1]);
	int len_to_write = strlen(av[2]);
	int	x_offset = x - ((x - 1 + len_to_write) % term_width);
	int	y_offset = (x - 1 + len_to_write) / term_width;

	printf("x_ffset = %d, y_offset = %d\n", x_offset, y_offset);
}
