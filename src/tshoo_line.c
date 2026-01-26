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

static void	replace_line(t_rl *rl, char *replacement) {
	int	temp;

	temp = rl->idx;
	while (temp > 0) {
		write(2, "\x1b[D", 3);
		temp--;
	}
	dprintf(2, "\x1b[%ldP", strlen(rl->line));
	memcpy(rl->line, replacement, strlen(replacement) + 1);
	rl->len = strlen(replacement);
	rl->idx = rl->len;
	write(2, rl->line, strlen(rl->line));
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
	rl->pos++;
	rl->abs_pos++;
	write(2, "\x1b[C", 3);
}

static void	cursor_backward(t_rl *rl) {
	if (rl->idx <= 0)
		return ;
	rl->idx--;
	rl->pos--;
	rl->abs_pos--;
	write(2, "\x1b[D", 3);
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
	char	*dest;
	char	*src;
	int		temp;

	dest = rl->line + rl->idx + 1;
	src = rl->line + rl->idx;
	memmove(dest, src, strlen(src) + 1);
	src[0] = c;
	write(2, src, strlen(src));
	temp = rl->len;
	while (temp > rl->idx) {
		write(2, "\x1b[D", 3);
		temp--;
	}
	if (rl->pos == rl->width - 1) {
		write(2, "\v\r", 2);
		rl->pos = 0;
		rl->idx++;
		rl->len++;
	} else {
		rl->idx++;
		rl->len++;
		rl->pos++;
	}
}

static void	backspace_handling(t_rl *rl) {
	int		temp;
	char	cmd[32];

	if (rl->idx <= 0)
		return ;
	memmove(rl->line + rl->idx - 1, rl->line + rl->idx , strlen(rl->line + rl->idx) + 1);
	if (rl->pos == 0) {
		sprintf(cmd, "\x1b[%dC", rl->width);
		write(2, cmd, strlen(cmd));
		write(2, "\x1b[A", 3);
		rl->pos = rl->width - 1;
		rl->idx--;
		rl->len--;
	} else {
		rl->idx--;
		rl->pos--;
		rl->len--;
	}
	write(2, "\x1b[D", 3);
	write(2, "\x1b[1P", 4);
	write(2, rl->line + rl->idx, strlen(rl->line + rl->idx));
	temp = rl->len;
	while (temp > rl->idx) {
		write(2, "\x1b[D", 3);
		temp--;
	}
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
	size_t	prompt_len = printed_len(prompt);

	setup_signals(&(ctxt->old_sa), &(ctxt->sa));
	enable_raw_mode(&(ctxt->termios));
	write(2, prompt, strlen(prompt));
<<<<<<< HEAD
	rl->idx = 0;
=======
	rl->pos = 0;
>>>>>>> 28b5265de2dc6a3ddeddb99c9709e18d84298528
	rl->len = 0;
	rl->width = get_term_width(); ///careful sometimes
	rl->width = 10;
	rl->pos = prompt_len;
	rl->line[0] = '\0';
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
