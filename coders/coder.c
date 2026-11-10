/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:46 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 22:53:09 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	take_both(t_env *env, t_coder *coder)
{
	if (!take_dongle(env, coder, coder->first_dongle))
		return (0);
	if (!take_dongle(env, coder, coder->second_dongle))
	{
		release_dongle(env, coder->first_dongle);
		return (0);
	}
	return (1);
}

static int	all_done(t_env *env)
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

static int	announce_compile(t_env *env, t_coder *coder)
{
	int	ok;
	int	last;

	ok = 0;
	last = 0;
	pthread_mutex_lock(&env->log_mutex);
	pthread_mutex_lock(&env->state_mutex);
	if (!env->is_over)
	{
		ok = 1;
		coder->last_compile_start = get_time_ms();
		coder->compiles_count++;
		printf("%lld %d is compiling\n",
			coder->last_compile_start - env->start_time, coder->id);
		last = all_done(env);
		env->is_over = last;
	}
	pthread_mutex_unlock(&env->state_mutex);
	pthread_mutex_unlock(&env->log_mutex);
	if (last)
		wake_all(env);
	return (ok);
}

static int	coder_compile(t_env *env, t_coder *coder)
{
	int	ok;

	if (!take_both(env, coder))
		return (0);
	ok = announce_compile(env, coder);
	if (ok)
		compile_sleep(env->time_to_compile, env);
	release_dongle(env, coder->first_dongle);
	release_dongle(env, coder->second_dongle);
	return (ok);
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
		if (coder->compiles_count >= env->nb_compiles_required)
			break ;
		print_status(env, coder->id, "is debugging");
		ft_usleep(env->time_to_debug, env);
		print_status(env, coder->id, "is refactoring");
		ft_usleep(env->time_to_refactor, env);
	}
	return (NULL);
}
