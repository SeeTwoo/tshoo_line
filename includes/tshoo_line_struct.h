/* ************************************************************************** */
/*                                                                            */
/*                                                         :::     ::::::::   */
/*   ts_readline_struct.h                                :+:     :+:    :+:   */
/*                                                     +:+ +:+        +:+     */
/*   By: seetwoo <marvin@42students.fr>              +#+  +:+       +#+       */
/*                                                 +#+#+#+#+#+   +#+          */
/*   Created:                                           #+#    #+#            */
/*   Uptated:                                          ###   ########.fr      */
/*                                                                            */
/* ************************************************************************** */

#ifndef TSHOO_LINE_STRUCT_H 
# define TSHOO_LINE_STRUCT_H 

typedef struct s_rl	t_rl;

struct s_rl {
	char	line[1024];
	int		pos;
	int		len;
	int		width;
	int		abs_pos;
};

typedef struct s_tshoo_hist	t_tshoo_hist;

struct s_tshoo_hist {
	t_tshoo_hist	*prev;
	t_tshoo_hist	*next;
	char			*line;
};

#endif
