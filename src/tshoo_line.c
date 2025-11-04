#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
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

	temp = rl->i;
	while (temp > 0) {
		write(2, "\x1b[D", 3);
		temp--;
	}
	dprintf(2, "\x1b[%ldP", strlen(rl->line));
	memcpy(rl->line, replacement, strlen(replacement) + 1);
	rl->len = strlen(replacement);
	rl->i = rl->len;
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
	int	i;

	i = 0;
	while (1) {
		if (read(0, &cmd_buf[i], 1) == 0)
			continue ;
		if (isalpha(cmd_buf[i]))
			break ;
		i++;
	}
	*cmd = cmd_buf[i];
	i++;
	cmd_buf[i] = '\0';
}

static void	cursor_forward(t_rl *rl) {
	if (rl->i >= rl->len)
		return ;
	rl->i++;
	write(2, "\x1b[C", 3);
}

static void	cursor_backward(t_rl *rl) {
	if (rl->i <= 0)
		return ;
	rl->i--;
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

	dest = rl->line + rl->i + 1;
	src = rl->line + rl->i;
	memmove(dest, src, strlen(src) + 1);
	src[0] = c;
	write(2, src, strlen(src));
	temp = rl->len;
	while (temp > rl->i) {
		write(2, "\x1b[D", 3);
		temp--;
	}
	rl->i++;
	rl->len++;
}

static void	backspace_handling(t_rl *rl) {
	int	temp;

	if (rl->i <= 0)
		return ;
	memmove(rl->line + rl->i - 1, rl->line + rl->i , strlen(rl->line + rl->i) + 1);
	rl->i--;
	rl->len--;
	write(2, "\x1b[D", 3);
	write(2, "\x1b[1P", 4);
	write(2, rl->line + rl->i, strlen(rl->line + rl->i));
	temp = rl->len;
	while (temp > rl->i) {
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

static void	setup(t_rl *rl, t_ctxt *ctxt, char const *prompt) {
	setup_signals(&(ctxt->old_sa), &(ctxt->sa));
	enable_raw_mode(&(ctxt->termios));
	write(2, prompt, strlen(prompt));
	rl->i = 0;
	rl->len = 0;
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
