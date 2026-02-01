#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tshoo_line_struct.h"

static t_tshoo_hist	*new_hist_node(char *line, t_tshoo_hist *prev, t_tshoo_hist *next) {
	t_tshoo_hist	*new;

	new = malloc(sizeof(t_tshoo_hist));
	if (!new)
		return (NULL);
	new->prev = prev;
	new->next = next;
	new->line = strdup(line);
	if (!new->line)
		return (free(new), NULL);
	return (new);
}

int	tshoo_add_hist(char *line, t_tshoo_hist *history) {
	history->prev = new_hist_node(line, history->prev, history);
	if (!history->prev)
		dprintf(2, "Couldn't add the line [ %s ] to the history, malloc failed\n", line);
	if (history->prev->prev)
		history->prev->prev->next = history->prev;
	return (0);
}

void	tshoo_free_hist(t_tshoo_hist *history) {
	t_tshoo_hist	*temp;

	while (history) {
		temp = history;
		history = history->prev;
		free(temp->line);
		free(temp);
	}
}

t_tshoo_hist	*tshoo_init_hist(void) {
	t_tshoo_hist	*new;

	new = malloc(sizeof(t_tshoo_hist));
	if (!new)
		return (NULL);
	new->prev = NULL;
	new->next = NULL;
	new->line = strdup("");
	if (!new->line)
		return (free(new), NULL);
	return (new);
}
