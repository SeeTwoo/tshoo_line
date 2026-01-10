#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct termios		t_settings;

void	disable_raw_mode(t_settings *original) {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, original);
}

void	enable_raw_mode(t_settings *original) {
	t_settings	raw;

	tcgetattr(STDIN_FILENO, original);
	raw = *original;
	raw.c_lflag &= ~(ECHO | ICANON);
	raw.c_iflag &= ~(ICRNL | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag |= (ISIG);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
