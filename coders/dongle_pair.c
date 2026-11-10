/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_pair.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/10 10:00:00 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/10 10:00:00 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	push_one(t_env *env, t_dongle *d, t_req req)
{
	int	ok;

	pthread_mutex_lock(&d->mutex);
	ok = pq_push(&d->pq, req, env->is_edf);
	pthread_mutex_unlock(&d->mutex);
	return (ok);
}

static void	drop_one(t_env *env, t_dongle *d, int id)
{
	pthread_mutex_lock(&d->mutex);
	pq_remove(&d->pq, id, env->is_edf);
	pthread_mutex_unlock(&d->mutex);
}

static int	enqueue_pair(t_env *env, t_coder *c, t_dongle *lo, t_dongle *hi)
{
	t_req	req;

	req.coder_id = c->id;
	pthread_mutex_lock(&env->state_mutex);
	req.deadline = c->last_compile_start + env->time_to_burnout;
	req.req_seq = ++env->seq_counter;
	pthread_mutex_unlock(&env->state_mutex);
	if (!push_one(env, lo, req))
		return (0);
	if (!push_one(env, hi, req))
	{
		drop_one(env, lo, c->id);
		return (0);
	}
	return (1);
}

static int	wait_pair(t_env *env, t_coder *c, t_dongle *lo, t_dongle *hi)
{
	while (1)
	{
		pthread_mutex_lock(&lo->mutex);
		pthread_mutex_lock(&hi->mutex);
		if (simulation_over(env))
		{
			pthread_mutex_unlock(&hi->mutex);
			pthread_mutex_unlock(&lo->mutex);
			return (0);
		}
		if (try_pair(env, c, lo, hi))
			return (1);
	}
}

int	take_pair(t_env *env, t_coder *c)
{
	t_dongle	*lo;
	t_dongle	*hi;

	lo = &env->dongles[c->first_dongle];
	hi = &env->dongles[c->second_dongle];
	if (!enqueue_pair(env, c, lo, hi))
		return (0);
	if (!wait_pair(env, c, lo, hi))
	{
		drop_one(env, lo, c->id);
		drop_one(env, hi, c->id);
		return (0);
	}
	print_status(env, c->id, "has taken a dongle");
	print_status(env, c->id, "has taken a dongle");
	return (1);
}
