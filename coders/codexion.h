/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:46 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 18:00:00 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/time.h>
# include <unistd.h>

typedef struct s_req
{
	int			coder_id;
	long long	deadline;
	long long	req_seq;
}	t_req;

typedef struct s_pq
{
	t_req	*heap;
	int		capacity;
	int		size;
}	t_pq;

typedef struct s_dongle
{
	int				id;
	int				owner;
	long long		cooldown_until;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	t_pq			pq;
}	t_dongle;

typedef struct s_coder
{
	int				id;
	int				left_dongle;
	int				right_dongle;
	int				first_dongle;
	int				second_dongle;
	long long		last_compile_start;
	int				compiles_count;
	pthread_t		thread;
	struct s_env	*env;
}	t_coder;

typedef struct s_env
{
	int				nb_coders;
	long long		time_to_burnout;
	long long		time_to_compile;
	long long		time_to_debug;
	long long		time_to_refactor;
	int				nb_compiles_required;
	long long		dongle_cooldown;
	int				is_edf;
	long long		start_time;
	int				is_over;
	long long		seq_counter;
	int				nb_dongles;
	int				nb_started;
	int				monitor_started;
	pthread_mutex_t	log_mutex;
	pthread_mutex_t	state_mutex;
	t_dongle		*dongles;
	t_coder			*coders;
	pthread_t		monitor_thread;
}	t_env;

long long	get_time_ms(void);
void		ft_usleep(long long ms, t_env *env);

int			parse_args(int argc, char **argv, t_env *env);

int			pq_init(t_pq *pq, int capacity);
void		pq_destroy(t_pq *pq);
int			pq_push(t_pq *pq, t_req req, int is_edf);
t_req		pq_pop(t_pq *pq, int is_edf);
int			pq_peek_coder(t_pq *pq);
void		pq_remove(t_pq *pq, int coder_id, int is_edf);
int			is_higher(t_req a, t_req b, int is_edf);
void		sift_up(t_pq *pq, int i, int is_edf);
void		sift_down(t_pq *pq, int i, int is_edf);

int			init_dongles(t_env *env);
void		destroy_dongles(t_env *env);
int			take_dongle(t_env *env, t_coder *coder, int dongle_idx);
void		release_dongle(t_env *env, int dongle_idx);

int			start_simulation(t_env *env);
void		*coder_routine(void *arg);
void		*monitor_routine(void *arg);
void		cleanup_env(t_env *env);

int			simulation_over(t_env *env);
void		wake_all(t_env *env);
void		stop_simulation(t_env *env);
void		print_status(t_env *env, int id, const char *status);
void		print_burnout(t_env *env, int id);

#endif
