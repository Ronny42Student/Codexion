#include "codexion.h"

static void	init_env_defaults(t_env *env)
{
	memset(env, 0, sizeof(t_env));
	pthread_mutex_init(&env->log_mutex, NULL);
	pthread_mutex_init(&env->state_mutex, NULL);
}

int	main(int argc, char **argv)
{
	t_env	env;

	init_env_defaults(&env);
	if (!parse_args(argc, argv, &env))
	{
		write(2, "Error: Invalid arguments\n", 25);
		return (1);
	}
	if (!init_dongles(&env))
	{
		cleanup_env(&env);
		return (1);
	}
	if (!start_simulation(&env))
	{
		cleanup_env(&env);
		return (1);
	}
	cleanup_env(&env);
	return (0);
}
