/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:41 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 16:23:42 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	coder_compile(t_env *env, t_coder *coder)
{
	int	first;
	int	second;

	first = coder->left_dongle;
	second = coder->right_dongle;
	if (first > second)
	{
		first = coder->right_dongle;
		second = coder->left_dongle;
	}
	if (!take_dongle(env, coder, first))
		return (0);
	if (!take_dongle(env, coder, second))
	{
		release_dongle(env, first);
		return (0);
	}
	print_status(env, coder->id, "is compiling");
	pthread_mutex_lock(&env->state_mutex);
	coder->last_compile_start = get_time_ms();
	pthread_mutex_unlock(&env->state_mutex);
	ft_usleep(env->time_to_compile, env);
	release_dongle(env, first);
	release_dongle(env, second);
	coder->compiles_count++;
	return (1);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;
	t_env	*env;

	coder = (t_coder *)arg;
	env = coder->env;
	while (1)
	{
		pthread_mutex_lock(&env->state_mutex);
		if (env->is_over)
		{
			pthread_mutex_unlock(&env->state_mutex);
			break ;
		}
		pthread_mutex_unlock(&env->state_mutex);
		if (!coder_compile(env, coder))
			break ;
		print_status(env, coder->id, "is debugging");
		ft_usleep(env->time_to_debug, env);
		print_status(env, coder->id, "is refactoring");
		ft_usleep(env->time_to_refactor, env);
	}
	return (NULL);
}

static int	init_coders(t_env *env)
{
	int	i;

	env->coders = malloc(sizeof(t_coder) * env->nb_coders);
	if (!env->coders)
		return (0);
	i = 0;
	while (i < env->nb_coders)
	{
		env->coders[i].id = i + 1;
		env->coders[i].left_dongle = i;
		env->coders[i].right_dongle = (i + 1) % env->nb_coders;
		env->coders[i].last_compile_start = env->start_time;
		env->coders[i].compiles_count = 0;
		env->coders[i].env = env;
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
	i = 0;
	while (i < env->nb_coders)
	{
		if (pthread_create(&env->coders[i].thread, NULL, coder_routine,
				&env->coders[i]) != 0)
			return (0);
		i++;
	}
	return (1);
}

void	cleanup_env(t_env *env)
{
	int	i;

	if (env->coders)
	{
		i = 0;
		while (i < env->nb_coders)
		{
			pthread_join(env->coders[i].thread, NULL);
			i++;
		}
		pthread_join(env->monitor_thread, NULL);
		free(env->coders);
		env->coders = NULL;
	}
	destroy_dongles(env);
	pthread_mutex_destroy(&env->log_mutex);
	pthread_mutex_destroy(&env->state_mutex);
}
