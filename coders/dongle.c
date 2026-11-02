/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:46 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 18:00:00 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	init_one_dongle(t_dongle *d, int id, int capacity)
{
	d->id = id;
	d->owner = -1;
	d->cooldown_until = 0;
	if (pthread_mutex_init(&d->mutex, NULL) != 0)
		return (0);
	if (pthread_cond_init(&d->cond, NULL) != 0)
	{
		pthread_mutex_destroy(&d->mutex);
		return (0);
	}
	if (!pq_init(&d->pq, capacity))
	{
		pthread_cond_destroy(&d->cond);
		pthread_mutex_destroy(&d->mutex);
		return (0);
	}
	return (1);
}

int	init_dongles(t_env *env)
{
	int	i;

	env->dongles = malloc(sizeof(t_dongle) * env->nb_coders);
	if (!env->dongles)
		return (0);
	i = 0;
	while (i < env->nb_coders)
	{
		if (!init_one_dongle(&env->dongles[i], i, env->nb_coders))
		{
			destroy_dongles(env);
			return (0);
		}
		env->nb_dongles++;
		i++;
	}
	return (1);
}

void	destroy_dongles(t_env *env)
{
	int	i;

	if (!env->dongles)
		return ;
	i = 0;
	while (i < env->nb_dongles)
	{
		pthread_mutex_destroy(&env->dongles[i].mutex);
		pthread_cond_destroy(&env->dongles[i].cond);
		pq_destroy(&env->dongles[i].pq);
		i++;
	}
	free(env->dongles);
	env->dongles = NULL;
	env->nb_dongles = 0;
}

void	release_dongle(t_env *env, int dongle_idx)
{
	t_dongle	*d;

	d = &env->dongles[dongle_idx];
	pthread_mutex_lock(&d->mutex);
	d->owner = -1;
	d->cooldown_until = get_time_ms() + env->dongle_cooldown;
	pthread_cond_broadcast(&d->cond);
	pthread_mutex_unlock(&d->mutex);
}
