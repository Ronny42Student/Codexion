/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   time.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:46 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 16:23:47 by nrajaoar         ###   ########.fr       */
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
	while (1)
	{
		pthread_mutex_lock(&env->state_mutex);
		if (env->is_over)
		{
			pthread_mutex_unlock(&env->state_mutex);
			break ;
		}
		pthread_mutex_unlock(&env->state_mutex);
		if (get_time_ms() - start >= ms)
			break ;
		usleep(500);
	}
}
