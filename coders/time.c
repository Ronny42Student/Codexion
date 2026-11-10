/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   time.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:46 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 18:00:00 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long long	get_time_ms(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long long)tv.tv_sec * 1000 + (long long)tv.tv_usec / 1000);
}

void	ft_usleep(long long ms, t_env *env)
{
	long long	start;

	start = get_time_ms();
	while (!simulation_over(env) && get_time_ms() - start < ms)
		usleep(500);
}

static int	is_aborted(t_env *env)
{
	int	aborted;

	pthread_mutex_lock(&env->state_mutex);
	aborted = env->aborted;
	pthread_mutex_unlock(&env->state_mutex);
	return (aborted);
}

void	compile_sleep(long long ms, t_env *env)
{
	long long	start;

	start = get_time_ms();
	while (!is_aborted(env) && get_time_ms() - start < ms)
		usleep(500);
}
