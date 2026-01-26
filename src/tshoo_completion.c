#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#include "tshoo_line_struct.h"

#ifndef DELIM
# define DELIM "<>&| "
#endif

typedef struct s_comp	t_comp;

struct s_comp {
	char	*word_start;
	char	*word_end;
	char	*last_slash;
	char	*base;
	size_t	base_len;
	char	*filler;
};

static int	is_delim(char c) {
	return (c == '>' || c == '<' || c == '|' || c == '&' || c == ' ');
}

static char	*find_match(t_comp *comp) {
	char			*dir_name;
	DIR				*dir;
	struct dirent	*entry;

	if (comp->last_slash)
		dir_name = strndup(comp->word_start, comp->last_slash + 1 - comp->word_start);
	else
		dir_name = strdup(".");
	if (!dir_name)
		return (NULL);
	dir = opendir(dir_name);
	free(dir_name);
	if (!dir)
		return (NULL);
	entry = readdir(dir);
	while (entry) {
		if (strncmp(comp->base, entry->d_name, comp->base_len) == 0 || comp->base_len == 0)
			return (closedir(dir), entry->d_name + comp->base_len);
		entry = readdir(dir);
	}
	closedir(dir);
	return (NULL);
}

static void	init_comp(t_comp *comp, t_rl *rl) {
	int	temp = rl->idx;

	while (temp > 0 && !is_delim(rl->line[temp - 1]))
		temp--;
	comp->word_start = rl->line + temp;
	comp->last_slash = NULL;
	while (rl->line[temp] && !is_delim(rl->line[temp])) {
		if (rl->line[temp] == '/')
			comp->last_slash = rl->line + temp;
		temp++;
	}
	comp->word_end = rl->line + temp;
	comp->base = comp->last_slash ? comp->last_slash + 1 : comp->word_start;
	comp->base_len = comp->word_end - comp->base;
	comp->filler = find_match(comp);
}

static void	replace(t_rl *rl, t_comp *comp) {
	size_t	filler_len = strlen(comp->filler);

	memmove(comp->word_end + filler_len, comp->word_end, strcspn(comp->word_end, DELIM));
	memcpy(comp->word_end, comp->filler, filler_len);
	rl->idx += (comp->word_end + filler_len) - rl->line;
	rl->len += filler_len;
	rl->line[rl->len] = '\0';
	write(2, comp->filler, filler_len);
}

void	tshoo_completion(t_rl *rl) {
	t_comp	comp;

	init_comp(&comp, rl);
	if (!comp.filler)
		return ;
	replace(rl, &comp);
}
