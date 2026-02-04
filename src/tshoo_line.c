#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "tshoo_line_struct.h"

static volatile sig_atomic_t	got_sigint = 0;

typedef struct termios		t_settings;
typedef struct sigaction	t_sigaction;
typedef struct s_ctxt		t_ctxt;

struct s_ctxt {
	struct termios		termios;
	struct sigaction	sa;
	struct sigaction	old_sa;
};

void	enable_raw_mode(t_settings *original);
void	disable_raw_mode(t_settings *original);
void	tshoo_completion(t_rl *rl);

static int	compute_horizontal_offset(t_rl *rl, int len_to_write) {
	int	total = len_to_write + rl->x;
	int	final_x;

	if (total % rl->term_width == 0)
		final_x = rl->term_width - 1;
	else
		final_x = total % rl->term_width;
	return rl->x - final_x;
}

static int	compute_vertical_offset(t_rl *rl, int len_to_write) {
	int	total = len_to_write + rl->x;

	if (total % rl->term_width == 0)
		return (total / rl->term_width) - 1;
	else
		return total / rl->term_width;
}

static void	wrapper_delete(t_rl *rl) {
	int	y_offset = compute_vertical_offset(rl, rl->len - rl->idx);
	
	write(2, "\x1b[0K", 4);
	for (int i = 0; i < y_offset; i++) {
		write(2, "\x1b[B", 3);
		write(2, "\x1b[2K", 4);
	}
	if (y_offset > 0)
		dprintf(2, "\x1b[%dA", y_offset);
}

static void	wrapper_write(t_rl *rl) {
	int		len_to_write = rl->len - rl->idx;
	int		x_offset = compute_horizontal_offset(rl, len_to_write);
	int		y_offset = compute_vertical_offset(rl, len_to_write);
	int		bit_len;
	char	*temp = rl->line + rl->idx;

	if (len_to_write + rl->x > rl->term_width)
		bit_len = rl->term_width - rl->x;
	else
		bit_len = len_to_write;
	do {
		write(2, temp, bit_len);
		len_to_write -= bit_len;
		temp += bit_len;
		if (len_to_write)
			write(2, "\v\r", 2);
		bit_len = len_to_write > rl->term_width ? rl->term_width : len_to_write;
	} while (len_to_write);
	if (x_offset == 0 && y_offset <= 0)
		return ;
	if (y_offset > 0)
		dprintf(2, "\x1b[%dA", y_offset);
	if (x_offset > 0)
		dprintf(2, "\x1b[%dC", x_offset);
	else if (x_offset < 0)
		dprintf(2, "\x1b[%dD", -x_offset);
}

//TODO handle wrapping properly on rewwrite
static void	replace_line(t_rl *rl, char *replacement) {
	int	temp;

	temp = rl->idx;
	while (temp > 0) {
		write(2, "\x1b[D", 3);
		temp--;
	}
	dprintf(2, "\x1b[%dP", rl->len);
	rl->len = strlen(replacement);
	memcpy(rl->line, replacement, rl->len);
	rl->idx = rl->len;
	rl->x = rl->prompt_len + rl->len - 1;
	write(2, rl->line, rl->len);
}

static void	replace_from_history(t_rl *rl, t_tshoo_hist **history, char cmd) {
	char	*replacement;

	if (*history == NULL)
		return ;
	if (cmd == 'A' && (*history)->prev)
		*history = (*history)->prev;
	else if (cmd == 'B' && (*history)->next)
		*history = (*history)->next;
	else
		return ;
	if (!(*history)->line)
		return ;
	replacement = (*history)->line;
	replace_line(rl, replacement);
}

static void	fill_command(char *cmd_buf, char *cmd) {
	int	idx;

	idx = 0;
	while (1) {
		if (read(0, &cmd_buf[idx], 1) == 0)
			continue ;
		if (isalpha(cmd_buf[idx]))
			break ;
		idx++;
	}
	*cmd = cmd_buf[idx];
	idx++;
	cmd_buf[idx] = '\0';
}

static void	cursor_forward(t_rl *rl) {
	if (rl->idx >= rl->len)
		return ;
	rl->idx++;
	if (rl->x == rl->term_width - 1) {
		rl->x = 0;
		write(2, "\v\r", 2);
	} else {
		rl->x++;
		write(2, "\x1b[C", 3);
	}
}

static void	cursor_backward(t_rl *rl) {
	if (rl->idx <= 0)
		return ;
	rl->idx--;
	if (rl->x == 0) {
		rl->x = rl->term_width - 1;
		write(2, "\x1b[A", 3);
		dprintf(2, "\x1b[%dC", rl->term_width - 1);
	} else {
		rl->x--;
		write(2, "\x1b[D", 3);
	}
}

static void	arrow_handling(t_rl *rl, t_tshoo_hist **history) {
	char	cmd_buf[32];
	char	cmd;

	fill_command(cmd_buf, &cmd);
	if (cmd == 'A' || cmd == 'B')
		replace_from_history(rl, history, cmd);
	else if (cmd == 'C')
		cursor_forward(rl);
	else if (cmd == 'D')
		cursor_backward(rl);
}

static void	fill_line(t_rl *rl, char c) {
	char	*current = rl->line + rl->idx;

	memmove(current + 1, current,  rl->len - rl->idx);
	*current = c;
	rl->len++;
	wrapper_write(rl);
	cursor_forward(rl);
}

static void	backspace_handling(t_rl *rl) {
	if (rl->idx <= 0)
		return ;

	char	*current = rl->line + rl->idx;

	memmove(current - 1, current, rl->len - rl->idx);
	cursor_backward(rl);
	rl->len--;
	wrapper_delete(rl);
	wrapper_write(rl);
}

static void	control_c(t_rl *rl) {
	replace_line(rl, "");
	got_sigint = 0;
}

static void	line_sigint(int sig) {
	(void)sig;
	got_sigint = 1;
}

static void	setup_signals(t_sigaction *old, t_sigaction *sa) {
	sa->sa_handler = line_sigint;
	sigemptyset(&sa->sa_mask);
	sa->sa_flags = 0;
	sigaction(SIGINT, sa, old);
}

static int	get_term_width(void) {
	struct winsize	ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
		return 40;
	return ws.ws_col;
}

static size_t	printed_len(char const *s) {
	size_t	i = 0;

	while (*s) {
		if (*s == '\x1b') {
			while (!isalpha(*s) && *s)
				s++;
			if (isalpha(*s))
				s++;
		} else {
			s++;
			i++;
		}
	}
	return i;
}

static void	setup(t_rl *rl, t_ctxt *ctxt, char const *prompt) {
	setup_signals(&(ctxt->old_sa), &(ctxt->sa));
	enable_raw_mode(&(ctxt->termios));
	write(2, prompt, strlen(prompt));
	rl->idx = 0;
	rl->len = 0;
	rl->term_width = get_term_width();
	//rl->term_width = 10;
	rl->prompt_len = printed_len(prompt);
	rl->x = rl->prompt_len;
	rl->y = 0;
	rl->row_nbr = 1;
}

static void	cleanup(t_rl *rl, t_ctxt *ctxt) {
	rl->line[rl->len] = '\0';
	write(2, "\r\v", 2);
	disable_raw_mode(&(ctxt->termios));
	sigaction(SIGINT, &(ctxt->old_sa), NULL);
}

char	*tshoo_line(char const *prompt, t_tshoo_hist *history) {
	t_ctxt	ctxt;
	char	c;
	t_rl	rl;
	ssize_t	bytes_read;

	setup(&rl, &ctxt, prompt);
	while (1) {
		bytes_read = read(0, &c, 1);
		if (bytes_read < 0 && errno == EINTR && got_sigint)
			control_c(&rl);
		else if (c == '\x4' && rl.len == 0)
			return (cleanup(&rl, &ctxt), NULL);
		else if (bytes_read == 0 || c == '\x4')
			continue ;
		else if (c == '\r' || rl.len == 1023)
			break ;
		else if (c == '\x7f')
			backspace_handling(&rl);
		else if (c == '\x1b')
			arrow_handling(&rl, &history);
		else if (c == '\t')
			tshoo_completion(&rl);
		else
			fill_line(&rl, c);
	}
	cleanup(&rl, &ctxt);
	return (strdup(rl.line));
}
