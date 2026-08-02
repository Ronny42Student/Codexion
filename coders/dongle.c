#include "codexion.h"

int	init_dongles(t_env *env)
{
	int	i;

	env->dongles = malloc(sizeof(t_dongle) * env->nb_coders);
	if (!env->dongles)
		return (0);
	i = 0;
	while (i < env->nb_coders)
	{
		env->dongles[i].id = i;
		env->dongles[i].owner = -1;
		env->dongles[i].cooldown_until = 0;
		if (pthread_mutex_init(&env->dongles[i].mutex, NULL) != 0)
			return (0);
		if (pthread_cond_init(&env->dongles[i].cond, NULL) != 0)
			return (0);
		if (!pq_init(&env->dongles[i].pq, env->nb_coders))
			return (0);
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
	while (i < env->nb_coders)
	{
		pthread_mutex_destroy(&env->dongles[i].mutex);
		pthread_cond_destroy(&env->dongles[i].cond);
		pq_destroy(&env->dongles[i].pq);
		i++;
	}
	free(env->dongles);
	env->dongles = NULL;
}

static void	build_timespec(long long abs_ms, struct timespec *ts)
{
	ts->tv_sec = abs_ms / 1000;
	ts->tv_nsec = (abs_ms % 1000) * 1000000;
}

int	take_dongle(t_env *env, t_coder *coder, int dongle_idx)
{
	t_dongle		*d;
	t_req			req;
	struct timespec	ts;

	d = &env->dongles[dongle_idx];
	pthread_mutex_lock(&d->mutex);
	req.coder_id = coder->id;
	req.deadline = coder->last_compile_start + env->time_to_burnout;
	pthread_mutex_lock(&env->state_mutex);
	req.req_seq = ++env->seq_counter;
	pthread_mutex_unlock(&env->state_mutex);
	pq_push(&d->pq, req, env->is_edf);
	while (1)
	{
		pthread_mutex_lock(&env->state_mutex);
		if (env->is_over)
		{
			pthread_mutex_unlock(&env->state_mutex);
			pq_remove(&d->pq, coder->id, env->is_edf);
			pthread_mutex_unlock(&d->mutex);
			return (0);
		}
		pthread_mutex_unlock(&env->state_mutex);
		if (d->owner == -1 && pq_peek_coder(&d->pq) == coder->id)
		{
			if (get_time_ms() >= d->cooldown_until)
			{
				d->owner = coder->id;
				pq_pop(&d->pq, env->is_edf);
				pthread_mutex_unlock(&d->mutex);
				print_status(env, coder->id, "has taken a dongle");
				return (1);
			}
			build_timespec(d->cooldown_until, &ts);
			pthread_cond_timedwait(&d->cond, &d->mutex, &ts);
		}
		else
			pthread_cond_wait(&d->cond, &d->mutex);
	}
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
