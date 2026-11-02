/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_take.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:46 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 18:00:00 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	build_timespec(long long abs_ms, struct timespec *ts)
{
	ts->tv_sec = abs_ms / 1000;
	ts->tv_nsec = (abs_ms % 1000) * 1000000;
}

static int	enqueue_request(t_env *env, t_coder *coder, t_dongle *d)
{
	t_req	req;

	req.coder_id = coder->id;
	pthread_mutex_lock(&env->state_mutex);
	req.deadline = coder->last_compile_start + env->time_to_burnout;
	req.req_seq = ++env->seq_counter;
	pthread_mutex_unlock(&env->state_mutex);
	return (pq_push(&d->pq, req, env->is_edf));
}

static int	try_grab(t_env *env, t_coder *coder, t_dongle *d)
{
	struct timespec	ts;

	if (d->owner == -1 && pq_peek_coder(&d->pq) == coder->id)
	{
		if (get_time_ms() >= d->cooldown_until)
		{
			d->owner = coder->id;
			pq_pop(&d->pq, env->is_edf);
			return (1);
		}
		build_timespec(d->cooldown_until, &ts);
		pthread_cond_timedwait(&d->cond, &d->mutex, &ts);
	}
	else
		pthread_cond_wait(&d->cond, &d->mutex);
	return (0);
}

int	take_dongle(t_env *env, t_coder *coder, int dongle_idx)
{
	t_dongle	*d;

	d = &env->dongles[dongle_idx];
	pthread_mutex_lock(&d->mutex);
	if (!enqueue_request(env, coder, d))
	{
		pthread_mutex_unlock(&d->mutex);
		return (0);
	}
	while (!simulation_over(env))
	{
		if (try_grab(env, coder, d))
		{
			pthread_mutex_unlock(&d->mutex);
			print_status(env, coder->id, "has taken a dongle");
			return (1);
		}
	}
	pq_remove(&d->pq, coder->id, env->is_edf);
	pthread_mutex_unlock(&d->mutex);
	return (0);
}
