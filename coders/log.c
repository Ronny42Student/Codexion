/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:46 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 18:00:00 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	simulation_over(t_env *env)
{
	int	over;

	pthread_mutex_lock(&env->state_mutex);
	over = env->is_over;
	pthread_mutex_unlock(&env->state_mutex);
	return (over);
}

void	wake_all(t_env *env)
{
	int	i;

	i = 0;
	while (i < env->nb_dongles)
	{
		pthread_mutex_lock(&env->dongles[i].mutex);
		pthread_cond_broadcast(&env->dongles[i].cond);
		pthread_mutex_unlock(&env->dongles[i].mutex);
		i++;
	}
}

void	stop_simulation(t_env *env)
{
	pthread_mutex_lock(&env->state_mutex);
	env->is_over = 1;
	env->aborted = 1;
	pthread_mutex_unlock(&env->state_mutex);
	wake_all(env);
}

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
	int			first;

	first = 0;
	pthread_mutex_lock(&env->log_mutex);
	pthread_mutex_lock(&env->state_mutex);
	if (!env->is_over)
	{
		env->is_over = 1;
		env->aborted = 1;
		first = 1;
		now = get_time_ms() - env->start_time;
		printf("%lld %d burned out\n", now, id);
	}
	pthread_mutex_unlock(&env->state_mutex);
	pthread_mutex_unlock(&env->log_mutex);
	if (first)
		wake_all(env);
}
