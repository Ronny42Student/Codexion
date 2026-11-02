/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:34 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 16:23:35 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	print_status(t_env *env, int id, const char *status)
{
	long long	now;

	pthread_mutex_lock(&env->log_mutex);
	pthread_mutex_lock(&env->state_mutex);
	if (!env->is_over)
	{
		now = get_time_ms() - env->start_time;
		printf("%lld %d %s\n", now, id, status);
	}
	pthread_mutex_unlock(&env->state_mutex);
	pthread_mutex_unlock(&env->log_mutex);
}

void	print_burnout(t_env *env, int id)
{
	long long	now;
	int			i;

	pthread_mutex_lock(&env->log_mutex);
	pthread_mutex_lock(&env->state_mutex);
	if (!env->is_over)
	{
		env->is_over = 1;
		now = get_time_ms() - env->start_time;
		printf("%lld %d burned out\n", now, id);
		pthread_mutex_unlock(&env->state_mutex);
		i = 0;
		while (i < env->nb_coders)
		{
			pthread_mutex_lock(&env->dongles[i].mutex);
			pthread_cond_broadcast(&env->dongles[i].cond);
			pthread_mutex_unlock(&env->dongles[i].mutex);
			i++;
		}
		pthread_mutex_unlock(&env->log_mutex);
		return ;
	}
	pthread_mutex_unlock(&env->state_mutex);
	pthread_mutex_unlock(&env->log_mutex);
}

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

void	*monitor_routine(void *arg)
{
	t_env	*env;
	int		i;
	int		burned_id;

	env = (t_env *)arg;
	while (1)
	{
		pthread_mutex_lock(&env->state_mutex);
		if (env->is_over)
		{
			pthread_mutex_unlock(&env->state_mutex);
			break ;
		}
		if (check_compiles_done(env))
		{
			env->is_over = 1;
			pthread_mutex_unlock(&env->state_mutex);
			i = 0;
			while (i < env->nb_coders)
			{
				pthread_mutex_lock(&env->dongles[i].mutex);
				pthread_cond_broadcast(&env->dongles[i].cond);
				pthread_mutex_unlock(&env->dongles[i].mutex);
				i++;
			}
			break ;
		}
		burned_id = check_burnout_id(env);
		pthread_mutex_unlock(&env->state_mutex);
		if (burned_id > 0)
		{
			print_burnout(env, burned_id);
			break ;
		}
		usleep(1000);
	}
	return (NULL);
}
