
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
	rl->term_width = 10;
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
