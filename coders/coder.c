/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:46 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 22:08:48 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	single_dongle(t_env *env, t_coder *coder)
{
	if (take_dongle(env, coder, coder->first_dongle))
	{
		while (!simulation_over(env))
			usleep(500);
		release_dongle(env, coder->first_dongle);
	}
	return (0);
}

static int	take_both(t_env *env, t_coder *coder)
{
	if (coder->first_dongle == coder->second_dongle)
		return (single_dongle(env, coder));
	if (!take_dongle(env, coder, coder->first_dongle))
		return (0);
	if (!take_dongle(env, coder, coder->second_dongle))
	{
		release_dongle(env, coder->first_dongle);
		return (0);
	}
	return (1);
}

static int	coder_compile(t_env *env, t_coder *coder)
{
	if (!take_both(env, coder))
		return (0);
	pthread_mutex_lock(&env->state_mutex);
	coder->last_compile_start = get_time_ms();
	pthread_mutex_unlock(&env->state_mutex);
	print_status(env, coder->id, "is compiling");
	ft_usleep(env->time_to_compile, env);
	release_dongle(env, coder->first_dongle);
	release_dongle(env, coder->second_dongle);
	pthread_mutex_lock(&env->state_mutex);
	coder->compiles_count++;
	pthread_mutex_unlock(&env->state_mutex);
	return (1);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;
	t_env	*env;

	coder = (t_coder *)arg;
	env = coder->env;
	while (!simulation_over(env))
	{
		if (!coder_compile(env, coder))
			break ;
		print_status(env, coder->id, "is debugging");
		ft_usleep(env->time_to_debug, env);
		print_status(env, coder->id, "is refactoring");
		ft_usleep(env->time_to_refactor, env);
	}
	return (NULL);
}
