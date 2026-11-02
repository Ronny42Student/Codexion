/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parsing.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:38 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 16:23:39 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static long long	ft_atol(const char *str)
{
	long long	res;
	int			i;

	res = 0;
	i = 0;
	if (!str || !str[0])
		return (-1);
	while (str[i])
	{
		if (str[i] < '0' || str[i] > '9')
			return (-1);
		res = res * 10 + (str[i] - '0');
		if (res > 2147483647)
			return (-1);
		i++;
	}
	return (res);
}

static int	validate_scheduler(const char *str, int *is_edf)
{
	if (strcmp(str, "fifo") == 0)
	{
		*is_edf = 0;
		return (1);
	}
	if (strcmp(str, "edf") == 0)
	{
		*is_edf = 1;
		return (1);
	}
	return (0);
}

int	parse_args(int argc, char **argv, t_env *env)
{
	if (argc != 9)
		return (0);
	env->nb_coders = (int)ft_atol(argv[1]);
	env->time_to_burnout = ft_atol(argv[2]);
	env->time_to_compile = ft_atol(argv[3]);
	env->time_to_debug = ft_atol(argv[4]);
	env->time_to_refactor = ft_atol(argv[5]);
	env->nb_compiles_required = (int)ft_atol(argv[6]);
	env->dongle_cooldown = ft_atol(argv[7]);
	if (env->nb_coders <= 0 || env->time_to_burnout < 0
		|| env->time_to_compile < 0 || env->time_to_debug < 0
		|| env->time_to_refactor < 0 || env->nb_compiles_required < 0
		|| env->dongle_cooldown < 0)
		return (0);
	if (!validate_scheduler(argv[8], &env->is_edf))
		return (0);
	return (1);
}
