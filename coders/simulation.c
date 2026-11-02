/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:46 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 18:00:00 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	set_order(t_coder *c)
{
	if (c->left_dongle < c->right_dongle)
	{
		c->first_dongle = c->left_dongle;
		c->second_dongle = c->right_dongle;
	}
	else
	{
		c->first_dongle = c->right_dongle;
		c->second_dongle = c->left_dongle;
	}
}

static int	init_coders(t_env *env)
{
	int		i;
	t_coder	*c;

	env->coders = malloc(sizeof(t_coder) * env->nb_coders);
	if (!env->coders)
		return (0);
	i = 0;
	while (i < env->nb_coders)
	{
		c = &env->coders[i];
		c->id = i + 1;
		c->left_dongle = i;
		c->right_dongle = (i + 1) % env->nb_coders;
		set_order(c);
		c->last_compile_start = env->start_time;
		c->compiles_count = 0;
		c->env = env;
		i++;
	}
	return (1);
}

int	start_simulation(t_env *env)
{
	int	i;

	env->start_time = get_time_ms();
	if (!init_coders(env))
		return (0);
	if (pthread_create(&env->monitor_thread, NULL, monitor_routine, env) != 0)
		return (0);
	env->monitor_started = 1;
	i = 0;
	while (i < env->nb_coders)
	{
		if (pthread_create(&env->coders[i].thread, NULL, coder_routine,
				&env->coders[i]) != 0)
		{
			stop_simulation(env);
			return (0);
		}
		env->nb_started++;
		i++;
	}
	return (1);
}

void	cleanup_env(t_env *env)
{
	int	i;

	i = 0;
	while (i < env->nb_started)
	{
		pthread_join(env->coders[i].thread, NULL);
		i++;
	}
	if (env->monitor_started)
		pthread_join(env->monitor_thread, NULL);
	free(env->coders);
	env->coders = NULL;
	destroy_dongles(env);
	pthread_mutex_destroy(&env->log_mutex);
	pthread_mutex_destroy(&env->state_mutex);
}
