/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_wait.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/10 10:00:00 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/10 10:00:00 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	dongle_ok(t_dongle *d, long long now)
{
	return (d->owner == -1 && now >= d->cooldown_until);
}

static int	yields(t_env *env, t_coder *c, t_dongle *d)
{
	int		rival;
	int		other;
	int		busy;
	t_coder	*r;

	rival = pq_peek_coder(&d->pq);
	if (rival < 0 || rival == c->id)
		return (0);
	r = &env->coders[rival - 1];
	other = r->left_dongle;
	if (other == d->id)
		other = r->right_dongle;
	pthread_mutex_lock(&env->state_mutex);
	busy = (env->dongles[other].owner != -1);
	pthread_mutex_unlock(&env->state_mutex);
	return (!busy);
}

static void	grab_pair(t_env *env, t_coder *c, t_dongle *lo, t_dongle *hi)
{
	pthread_mutex_lock(&env->state_mutex);
	lo->owner = c->id;
	hi->owner = c->id;
	pthread_mutex_unlock(&env->state_mutex);
	pq_remove(&lo->pq, c->id, env->is_edf);
	pq_remove(&hi->pq, c->id, env->is_edf);
	pthread_cond_broadcast(&lo->cond);
	pthread_cond_broadcast(&hi->cond);
	pthread_mutex_unlock(&hi->mutex);
	pthread_mutex_unlock(&lo->mutex);
}

static void	wait_on(t_dongle *d, long long now)
{
	struct timespec	ts;
	long long		until;

	if (d->owner != -1)
	{
		pthread_cond_wait(&d->cond, &d->mutex);
		return ;
	}
	until = now + 1;
	if (d->cooldown_until > now)
		until = d->cooldown_until;
	ts.tv_sec = until / 1000;
	ts.tv_nsec = (until % 1000) * 1000000;
	pthread_cond_timedwait(&d->cond, &d->mutex, &ts);
}

int	try_pair(t_env *env, t_coder *c, t_dongle *lo, t_dongle *hi)
{
	long long	now;
	t_dongle	*blk;
	t_dongle	*other;

	now = get_time_ms();
	if (dongle_ok(lo, now) && dongle_ok(hi, now)
		&& !yields(env, c, lo) && !yields(env, c, hi))
	{
		grab_pair(env, c, lo, hi);
		return (1);
	}
	blk = lo;
	other = hi;
	if (dongle_ok(lo, now))
	{
		blk = hi;
		other = lo;
	}
	pthread_mutex_unlock(&other->mutex);
	wait_on(blk, now);
	pthread_mutex_unlock(&blk->mutex);
	return (0);
}
