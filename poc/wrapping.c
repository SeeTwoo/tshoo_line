#include <unistd.h>
#include <stdio.h>

int	main(void) {
	char	c;

	while (1) {
		read(0, &c, 1);
		if (c == '\n')
			break ;
	}
	return 0;
}
