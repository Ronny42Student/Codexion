/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:46 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/07 09:41:54 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	check_compiles_done(t_env *env)
{
	int	i;

	if (env->nb_compiles_required < 0)
		return (0);
	i = 0;
	while (i < env->nb_coders)
	{
		if (env->coders[i].compiles_count < env->nb_compiles_required)
			return (0);
		i++;
	}
	return (1);
}

static int	check_burnout_id(t_env *env)
{
	int			i;
	long long	now;

	now = get_time_ms();
	i = 0;
	while (i < env->nb_coders)
	{
		if (now - env->coders[i].last_compile_start > env->time_to_burnout)
			return (env->coders[i].id);
		i++;
	}
	return (0);
}

static int	monitor_check(t_env *env)
{
	int	res;

	pthread_mutex_lock(&env->state_mutex);
	if (check_compiles_done(env))
		res = -1;
	else
		res = check_burnout_id(env);
	pthread_mutex_unlock(&env->state_mutex);
	return (res);
}

void	*monitor_routine(void *arg)
{
	t_env	*env;
	int		res;

	env = (t_env *)arg;
	while (!simulation_over(env))
	{
		res = monitor_check(env);
		if (res < 0)
			stop_simulation(env);
		else if (res > 0)
			print_burnout(env, res);
		if (res != 0)
			break ;
		usleep(500);
	}
	return (NULL);
}
